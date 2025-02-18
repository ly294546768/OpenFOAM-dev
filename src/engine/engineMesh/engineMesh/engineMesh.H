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

Class
    Foam::engineMesh

Description
    Foam::engineMesh

SourceFiles
    engineMesh.C

\*---------------------------------------------------------------------------*/

#ifndef engineMesh_H
#define engineMesh_H

#include "fvMesh.H"
#include "autoPtr.H"
#include "runTimeSelectionTables.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class engineMesh Declaration
\*---------------------------------------------------------------------------*/

class engineMesh
:
    public fvMesh
{
protected:

    // Protected data

        const IOdictionary dict_;

        //- RPM
        dimensionedScalar rpm_;

        //- Optional engine geometry parameters
        dimensionedScalar conRodLength_;
        dimensionedScalar bore_;
        dimensionedScalar stroke_;
        dimensionedScalar clearance_;

        label pistonIndex_;
        label linerIndex_;
        label cylinderHeadIndex_;

        dimensionedScalar deckHeight_;
        dimensionedScalar pistonPosition_;


public:

    //- Runtime type information
    TypeName("engineMesh");


    // Declare run-time constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            engineMesh,
            IOobject,
            (const IOobject& io),
            (io)
        );


    // Constructors

        //- Construct from objectRegistry, and read/write options
        explicit engineMesh(const IOobject& io);

        //- Disallow default bitwise copy construction
        engineMesh(const engineMesh&) = delete;


    // Selectors

        //- Select null constructed
        static autoPtr<engineMesh> New(const IOobject& io);


    //- Destructor
    virtual ~engineMesh();


    // Member Functions

        //- Is mesh dynamic
        virtual bool dynamic() const
        {
            return true;
        }

        //- Return the engines current operating RPM
        const dimensionedScalar& rpm() const
        {
            return rpm_;
        }

        //- Return the engines connecting-rod length
        const dimensionedScalar& conRodLength() const
        {
            return conRodLength_;
        }

        //- Return the engines bore
        const dimensionedScalar& bore() const
        {
            return bore_;
        }

        //- Return the engines stroke
        const dimensionedScalar& stroke() const
        {
            return stroke_;
        }

        //- Return the engines clearance-gap
        const dimensionedScalar& clearance() const
        {
            return clearance_;
        }

        //- Return current crank-angle
        virtual scalar theta() const;

        //- Return crank-angle increment
        virtual scalar deltaTheta() const;

        //- Calculate the piston position from the engine geometry
        //  and given crank angle.
        scalar pistonPosition(const scalar theta) const;

        //- Return current piston position
        dimensionedScalar pistonPosition() const;

        //- Return piston displacement for current time step
        dimensionedScalar pistonDisplacement() const;

        //- Return piston speed for current time step
        dimensionedScalar pistonSpeed() const;


        virtual void move() = 0;


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const engineMesh&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
