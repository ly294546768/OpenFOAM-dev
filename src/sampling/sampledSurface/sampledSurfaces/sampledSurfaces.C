/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2021 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "sampledSurfaces.H"
#include "PatchTools.H"
#include "mapPolyMesh.H"
#include "OSspecific.H"
#include "writeFile.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(sampledSurfaces, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        sampledSurfaces,
        dictionary
    );
}
}

bool Foam::functionObjects::sampledSurfaces::verbose_ = false;
Foam::scalar Foam::functionObjects::sampledSurfaces::mergeTol_ = 1e-10;


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::sampledSurfaces::writeGeometry() const
{
    // Write to time directory under outputPath_
    // Skip surface without faces (eg, a failed cut-plane)

    const fileName outputDir = outputPath_/mesh_.time().timeName();

    forAll(*this, si)
    {
        const sampledSurface& s = operator[](si);

        if (Pstream::parRun())
        {
            if (Pstream::master() && mergeList_[si].faces.size())
            {
                formatter_->write
                (
                    outputDir,
                    s.name(),
                    mergeList_[si].points,
                    mergeList_[si].faces
                );
            }
        }
        else if (s.faces().size())
        {
            formatter_->write
            (
                outputDir,
                s.name(),
                s.points(),
                s.faces()
            );
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::sampledSurfaces::sampledSurfaces
(
    const word& name,
    const Time& t,
    const dictionary& dict
)
:
    functionObject(name),
    PtrList<sampledSurface>(),
    mesh_
    (
        refCast<const fvMesh>
        (
            t.lookupObject<objectRegistry>
            (
                dict.lookupOrDefault("region", polyMesh::defaultRegion)
            )
        )
    ),
    outputPath_(fileName::null),
    fields_(),
    interpolationScheme_(word::null),
    mergeList_(),
    formatter_(nullptr)
{
    outputPath_ =
        mesh_.time().globalPath()/functionObjects::writeFile::outputPrefix/name;

    if (mesh_.name() != fvMesh::defaultRegion)
    {
        outputPath_ = outputPath_/mesh_.name();
    }

    read(dict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::functionObjects::sampledSurfaces::~sampledSurfaces()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::functionObjects::sampledSurfaces::verbose(const bool verbosity)
{
    verbose_ = verbosity;
}


bool Foam::functionObjects::sampledSurfaces::execute()
{
    return true;
}


bool Foam::functionObjects::sampledSurfaces::write()
{
    if (size())
    {
        // Finalise surfaces, merge points etc.
        update();

        const label nFields = classifyFields();

        if (Pstream::master())
        {
            if (debug)
            {
                Pout<< "Creating directory "
                    << outputPath_/mesh_.time().timeName() << nl << endl;

            }

            mkDir(outputPath_/mesh_.time().timeName());
        }

        // Write geometry first if required,
        // or when no fields would otherwise be written
        if (nFields == 0 || formatter_->separateGeometry())
        {
            writeGeometry();
        }

        const IOobjectList objects(mesh_, mesh_.time().timeName());

        sampleAndWrite<volScalarField>(objects);
        sampleAndWrite<volVectorField>(objects);
        sampleAndWrite<volSphericalTensorField>(objects);
        sampleAndWrite<volSymmTensorField>(objects);
        sampleAndWrite<volTensorField>(objects);

        sampleAndWrite<surfaceScalarField>(objects);
        sampleAndWrite<surfaceVectorField>(objects);
        sampleAndWrite<surfaceSphericalTensorField>(objects);
        sampleAndWrite<surfaceSymmTensorField>(objects);
        sampleAndWrite<surfaceTensorField>(objects);
    }

    return true;
}


bool Foam::functionObjects::sampledSurfaces::read(const dictionary& dict)
{
    bool surfacesFound = dict.found("surfaces");

    if (surfacesFound)
    {
        dict.lookup("fields") >> fields_;

        dict.lookup("interpolationScheme") >> interpolationScheme_;
        const word writeType(dict.lookup("surfaceFormat"));

        // Define the surface formatter
        formatter_ = surfaceWriter::New(writeType, dict);

        PtrList<sampledSurface> newList
        (
            dict.lookup("surfaces"),
            sampledSurface::iNew(mesh_)
        );
        transfer(newList);

        if (Pstream::parRun())
        {
            mergeList_.setSize(size());
        }

        // Ensure all surfaces and merge information are expired
        expire();

        if (this->size())
        {
            Info<< "Reading surface description:" << nl;
            forAll(*this, si)
            {
                Info<< "    " << operator[](si).name() << nl;
            }
            Info<< endl;
        }
    }

    if (Pstream::master() && debug)
    {
        Pout<< "sample fields:" << fields_ << nl
            << "sample surfaces:" << nl << "(" << nl;

        forAll(*this, si)
        {
            Pout<< "  " << operator[](si) << endl;
        }
        Pout<< ")" << endl;
    }

    return true;
}


Foam::wordList Foam::functionObjects::sampledSurfaces::fields() const
{
    wordList fields(fields_);

    forAll(*this, si)
    {
        fields.append(operator[](si).fields());
    }

    return fields;
}


void Foam::functionObjects::sampledSurfaces::updateMesh(const mapPolyMesh& mpm)
{
    if (&mpm.mesh() == &mesh_)
    {
        expire();
    }

    // pointMesh and interpolation will have been reset in mesh.update
}


void Foam::functionObjects::sampledSurfaces::movePoints(const polyMesh& mesh)
{
    if (&mesh == &mesh_)
    {
        expire();
    }
}


void Foam::functionObjects::sampledSurfaces::readUpdate
(
    const polyMesh::readUpdateState state
)
{
    if (state != polyMesh::UNCHANGED)
    {
        expire();
    }
}


bool Foam::functionObjects::sampledSurfaces::needsUpdate() const
{
    forAll(*this, si)
    {
        if (operator[](si).needsUpdate())
        {
            return true;
        }
    }

    return false;
}


bool Foam::functionObjects::sampledSurfaces::expire()
{
    bool justExpired = false;

    forAll(*this, si)
    {
        if (operator[](si).expire())
        {
            justExpired = true;
        }

        // Clear merge information
        if (Pstream::parRun())
        {
            mergeList_[si].clear();
        }
    }

    // true if any surfaces just expired
    return justExpired;
}


bool Foam::functionObjects::sampledSurfaces::update()
{
    bool updated = false;

    if (!needsUpdate())
    {
        return updated;
    }

    // Serial: quick and easy, no merging required
    if (!Pstream::parRun())
    {
        forAll(*this, si)
        {
            if (operator[](si).update())
            {
                updated = true;
            }
        }

        return updated;
    }

    // Dimension as fraction of mesh bounding box
    scalar mergeDim = mergeTol_ * mesh_.bounds().mag();

    if (Pstream::master() && debug)
    {
        Pout<< nl << "Merging all points within "
            << mergeDim << " metre" << endl;
    }

    forAll(*this, si)
    {
        sampledSurface& s = operator[](si);

        if (s.update())
        {
            updated = true;
        }
        else
        {
            continue;
        }

        PatchTools::gatherAndMerge
        (
            mergeDim,
            primitivePatch
            (
                SubList<face>(s.faces(), s.faces().size()),
                s.points()
            ),
            mergeList_[si].points,
            mergeList_[si].faces,
            mergeList_[si].pointsMap
        );
    }

    return updated;
}


// ************************************************************************* //
