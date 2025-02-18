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

#include "enginePiston.H"
#include "engineMesh.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::enginePiston::enginePiston
(
    const polyMesh& mesh,
    const word& pistonPatchName,
    const autoPtr<coordinateSystem>& pistonCS,
    const scalar minLayer,
    const scalar maxLayer
)
:
    mesh_(refCast<const engineMesh>(mesh)),
    patchID_(pistonPatchName, mesh.boundaryMesh()),
    csPtr_(pistonCS),
    minLayer_(minLayer),
    maxLayer_(maxLayer)
{}


Foam::enginePiston::enginePiston
(
    const polyMesh& mesh,
    const dictionary& dict
)
:
    mesh_(refCast<const engineMesh>(mesh)),
    patchID_(dict.lookup("patch"), mesh.boundaryMesh()),
    csPtr_
    (
        coordinateSystem::New
        (
            mesh_,
            dict.subDict("coordinateSystem")
        )
    ),
    minLayer_(dict.lookup<scalar>("minLayer")),
    maxLayer_(dict.lookup<scalar>("maxLayer"))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::enginePiston::writeDict(Ostream& os) const
{
    os  << nl << token::BEGIN_BLOCK
        << "patch " << patchID_.name() << token::END_STATEMENT << nl
        << "minLayer " << minLayer_ << token::END_STATEMENT << nl
        << "maxLayer " << maxLayer_ << token::END_STATEMENT << nl
        << token::END_BLOCK << endl;
}


// ************************************************************************* //
