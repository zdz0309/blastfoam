/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2018 OpenFOAM Foundation
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

#ifndef blastThermalBaffle1DFvPatchScalarFields_H
#define blastThermalBaffle1DFvPatchScalarFields_H

#include "blastThermalBaffle1DFvPatchScalarField.H"
#include "solidThermoPhysicsTypes.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace blast
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    typedef thermalBaffle1DFvPatchScalarField
    <
        hConstSolidThermoPhysics
    >constSolid_thermalBaffle1DFvPatchScalarField;

    typedef thermalBaffle1DFvPatchScalarField
    <
        hPowerSolidThermoPhysics
    >expoSolid_thermalBaffle1DFvPatchScalarField;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

}
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
