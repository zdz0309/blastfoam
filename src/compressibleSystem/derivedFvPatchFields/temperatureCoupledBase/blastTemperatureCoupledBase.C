/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
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

#include "blastTemperatureCoupledBase.H"
#include "volFields.H"
#include "fluidThermoModel.H"
#include "solidThermoModel.H"
#include "blastCompressibleTurbulenceModel.H"

// * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * * //

namespace Foam
{
    template<>
    const char* Foam::NamedEnum
    <
        Foam::blast::temperatureCoupledBase::KMethodType,
        4
    >::names[] =
    {
        "fluidThermo",
        "solidThermo",
        "directionalSolidThermo",
        "lookup"
    };
}


const Foam::NamedEnum<Foam::blast::temperatureCoupledBase::KMethodType, 4>
    Foam::blast::temperatureCoupledBase::KMethodTypeNames_;


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::blast::temperatureCoupledBase::temperatureCoupledBase
(
    const fvPatch& patch,
    const word& calculationType,
    const word& kappaName,
    const word& alphaAniName
)
:
    patch_(patch),
    method_(KMethodTypeNames_[calculationType]),
    kappaName_(kappaName),
    alphaAniName_(alphaAniName)
{}


Foam::blast::temperatureCoupledBase::temperatureCoupledBase
(
    const fvPatch& patch,
    const dictionary& dict
)
:
    patch_(patch),
    method_(KMethodTypeNames_.read(dict.lookup("kappaMethod"))),
    kappaName_(dict.lookupOrDefault<word>("kappa", "none")),
    alphaAniName_(dict.lookupOrDefault<word>("alphaAni","Anialpha"))
{
    switch (method_)
    {
        case mtDirectionalSolidThermo:
        {
            if (!dict.found("alphaAni"))
            {
                FatalIOErrorInFunction(dict)
                    << "Did not find entry 'alphaAni'"
                       " required for 'kappaMethod' "
                    << KMethodTypeNames_[method_]
                    << exit(FatalIOError);
            }

            break;
        }

        case mtLookup:
        {
            if (!dict.found("kappa"))
            {
                FatalIOErrorInFunction(dict)
                    << "Did not find entry 'kappa'"
                       " required for 'kappaMethod' "
                    <<  KMethodTypeNames_[method_] << nl
                    << "    Please set 'kappa' to the name of a volScalarField"
                       " or volSymmTensorField"
                    << exit(FatalIOError);
            }

            break;
        }

        default:
            break;
    }
}


Foam::blast::temperatureCoupledBase::temperatureCoupledBase
(
    const fvPatch& patch,
    const temperatureCoupledBase& base
)
:
    patch_(patch),
    method_(base.method_),
    kappaName_(base.kappaName_),
    alphaAniName_(base.alphaAniName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::scalarField> Foam::blast::temperatureCoupledBase::kappa
(
    const scalarField& Tp
) const
{
    const fvMesh& mesh = patch_.boundaryMesh().mesh();
    const label patchi = patch_.index();
    word thermoName("basicThermo");

    switch (method_)
    {
        case mtFluidThermo:
        {
            typedef blast::turbulenceModel turbulenceModel;

            word turbName(turbulenceModel::propertiesName);

            if (mesh.foundObject<turbulenceModel>(turbName))
            {
                const turbulenceModel& turbModel =
                    mesh.lookupObject<turbulenceModel>(turbName);
                return turbModel.kappaEff(patchi);
            }
            else if (mesh.foundObject<fluidThermoModel>(thermoName))
            {
                const fluidThermoModel& thermo =
                    mesh.lookupObject<fluidThermoModel>(thermoName);
                return thermo.kappa(patchi);
            }
            else
            {
                FatalErrorInFunction
                    << "kappaMethod defined to employ "
                    << KMethodTypeNames_[method_]
                    << " method, but thermo package not available"
                    << exit(FatalError);
            }

            break;
        }

        case mtSolidThermo:
        {
            const solidThermoModel& thermo =
                mesh.lookupObject<solidThermoModel>(thermoName);
            return thermo.kappa(patchi);
            break;
        }

        case mtDirectionalSolidThermo:
        {
            const solidThermoModel& thermo =
                mesh.lookupObject<solidThermoModel>(thermoName);

            const symmTensorField& alphaAni =
                patch_.lookupPatchField<volSymmTensorField, scalar>
                (
                    alphaAniName_
                );

            const scalarField& rhop = thermo.rho().boundaryField()[patchi];
            const scalarField& ep = thermo.e().boundaryField()[patchi];

            const symmTensorField kappa(alphaAni*thermo.Cp(rhop, ep, Tp, patchi));

            const vectorField n(patch_.nf());

            return n & kappa & n;
        }

        case mtLookup:
        {
            if (mesh.foundObject<volScalarField>(kappaName_))
            {
                return patch_.lookupPatchField<volScalarField, scalar>
                (
                    kappaName_
                );
            }
            else if (mesh.foundObject<volSymmTensorField>(kappaName_))
            {
                const symmTensorField& KWall =
                    patch_.lookupPatchField<volSymmTensorField, scalar>
                    (
                        kappaName_
                    );

                const vectorField n(patch_.nf());

                return n & KWall & n;
            }
            else
            {
                FatalErrorInFunction
                    << "Did not find field " << kappaName_
                    << " on mesh " << mesh.name() << " patch " << patch_.name()
                    << nl
                    << "    Please set 'kappa' to the name of a volScalarField"
                       " or volSymmTensorField."
                    << exit(FatalError);
            }

            break;
        }

        default:
        {
            FatalErrorInFunction
                << "Unimplemented method " << KMethodTypeNames_[method_] << nl
                << "    Please set 'kappaMethod' to one of "
                << KMethodTypeNames_.toc()
                << " and 'kappa' to the name of the volScalar"
                << " or volSymmTensor field (if kappa=lookup)"
                << exit(FatalError);
        }
    }

    return scalarField(0);
}


void Foam::blast::temperatureCoupledBase::write(Ostream& os) const
{
    writeEntry(os, "kappaMethod", KMethodTypeNames_[method_]);
    writeEntry(os, "kappa", kappaName_);
    writeEntry(os, "alphaAni", alphaAniName_);
}


// ************************************************************************* //
