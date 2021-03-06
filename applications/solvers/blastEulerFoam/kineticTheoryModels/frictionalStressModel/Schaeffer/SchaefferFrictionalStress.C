/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2019 OpenFOAM Foundation
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

#include "SchaefferFrictionalStress.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace kineticTheoryModels
{
namespace frictionalStressModels
{
    defineTypeNameAndDebug(Schaeffer, 0);

    addToRunTimeSelectionTable
    (
        frictionalStressModel,
        Schaeffer,
        dictionary
    );
}
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::Schaeffer
(
    const dictionary& dict
)
:
    frictionalStressModel(dict),
    coeffDict_(dict.optionalSubDict(typeName + "Coeffs")),
    phi_("phi", dimless, coeffDict_),
    alphaMinFrictionByAlphap_
    (
        "alphaMinFrictionByAlphap",
        dimless,
        coeffDict_
    )
{
    phi_ *= constant::mathematical::pi/180.0;
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::~Schaeffer()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField>
Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::
frictionalPressure
(
    const volScalarField& alphap,
    const volScalarField& alphaMax
) const
{
    volScalarField alphaMinFriction(alphaMinFrictionByAlphap_*alphaMax);

    return
        dimensionedScalar(dimensionSet(1, -1, -2, 0, 0), 1e24)
       *pow(Foam::max(alphap - alphaMinFriction, scalar(0)), 10.0);
}


Foam::tmp<Foam::volScalarField>
Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::
frictionalPressurePrime
(
    const volScalarField& alphap,
    const volScalarField& alphaMax
) const
{
    volScalarField alphaMinFriction(alphaMinFrictionByAlphap_*alphaMax);

    return
        dimensionedScalar(dimensionSet(1, -1, -2, 0, 0), 1e25)
       *pow(Foam::max(alphap - alphaMinFriction, scalar(0)), 9.0);
}


Foam::tmp<Foam::volScalarField>
Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::nu
(
    const phaseModel& phase,
    const volScalarField& alphap,
    const volScalarField& alphaMax,
    const volScalarField& pf,
    const volSymmTensorField& D
) const
{
    volScalarField alphaMinFriction(alphaMinFrictionByAlphap_*alphaMax);

    tmp<volScalarField> tnu
    (
        new volScalarField
        (
            IOobject
            (
                "Schaeffer:nu",
                phase.mesh().time().timeName(),
                phase.mesh(),
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            phase.mesh(),
            dimensionedScalar(dimensionSet(0, 2, -1, 0, 0), 0)
        )
    );

    volScalarField& nuf = tnu.ref();

    forAll(D, celli)
    {
        if (alphap[celli] > alphaMinFriction[celli])
        {
            nuf[celli] =
                0.5*pf[celli]*sin(phi_.value())
               /(
                    sqrt((1.0/3.0)*sqr(tr(D[celli])) - invariantII(D[celli]))
                  + SMALL
                );
        }
    }

    const fvPatchList& patches = phase.mesh().boundary();
    const volVectorField& U = phase.U();

    volScalarField::Boundary& nufBf = nuf.boundaryFieldRef();

    forAll(patches, patchi)
    {
        if (!patches[patchi].coupled())
        {
            nufBf[patchi] =
                (
                    pf.boundaryField()[patchi]*sin(phi_.value())
                   /(
                        mag(U.boundaryField()[patchi].snGrad())
                      + SMALL
                    )
                );
        }
    }

    // Correct coupled BCs
    nuf.correctBoundaryConditions();

    return tnu;
}


Foam::tmp<Foam::volScalarField>
Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::
alphaMinFriction
(
    const volScalarField& alphap,
    const volScalarField& alphaMax
) const
{
    return alphaMinFrictionByAlphap_*alphaMax;
}

bool Foam::kineticTheoryModels::frictionalStressModels::Schaeffer::read()
{
    coeffDict_ <<= dict_.optionalSubDict(typeName + "Coeffs");

    phi_.read(coeffDict_);
    phi_ *= constant::mathematical::pi/180.0;
    alphaMinFrictionByAlphap_.read(coeffDict_);

    return true;
}


// ************************************************************************* //
