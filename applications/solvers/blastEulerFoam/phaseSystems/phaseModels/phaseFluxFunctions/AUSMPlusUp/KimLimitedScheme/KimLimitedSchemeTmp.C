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

#include "KimLimitedScheme.H"
#include "surfaceFields.H"
#include "fvcGrad.H"
#include "coupledFvPatchFields.H"

// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

template<class Type, class Limiter, template<class> class LimitFunc>
void Foam::KimLimitedScheme<Type, Limiter, LimitFunc>::calcLimiter
(
    const GeometricField<Type, fvPatchField, volMesh>& phi,
    surfaceScalarField& limiterField
) const
{
    const fvMesh& mesh = this->mesh();

    tmp<GeometricField<typename Limiter::phiType, fvPatchField, volMesh>>
        tlPhi = LimitFunc<Type>()(phi);

    const GeometricField<typename Limiter::phiType, fvPatchField, volMesh>&
        lPhi = tlPhi();

    tmp<GeometricField<typename Limiter::gradPhiType, fvPatchField, volMesh>>
        tgradc(fvc::grad(lPhi));
    const GeometricField<typename Limiter::gradPhiType, fvPatchField, volMesh>&
        gradc = tgradc();

    volScalarField alphap(kineticTheory_.alphap());
    volScalarField alphaMinFriction(kineticTheory_.alphaMinFriction());
    volScalarField alphaMax(kineticTheory_.alphaMax());
    (
        (kineticTheory_.alphap() - kineticTheory_.alphaMinFriction())
       /(kineticTheory_.alphaMax() - kineticTheory_.alphaMinFriction())
    );

    const surfaceScalarField& CDweights = mesh.surfaceInterpolation::weights();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const vectorField& C = mesh.C();

    scalarField& pLim = limiterField.primitiveFieldRef();

    forAll(pLim, face)
    {
        label own = owner[face];
        label nei = neighbour[face];
        scalar alphaM(max(alphap[own], alphap[nei]));
        scalar alphaMinFric(min(alphaMinFriction[own], alphaMinFriction[nei]));
        scalar alphaMaxi(min(alphaMax[own], alphaMax[nei]));
        scalar zeta
        (
            max
            (
                (alphaM - alphaMinFric)/(alphaMaxi - alphaMinFric),
                0.0
            )
        );
        scalar G(max(2.0*(1.0 - D_*sqr(zeta)), 0.0));

        pLim[face] = limiter
        (
            CDweights[face],
            this->faceFlux_[face],
            lPhi[own],
            lPhi[nei],
            gradc[own],
            gradc[nei],
            C[nei] - C[own],
            G
        );
    }

    surfaceScalarField::Boundary& bLim =
        limiterField.boundaryFieldRef();

    forAll(bLim, patchi)
    {
        scalarField& pLim = bLim[patchi];

        if (bLim[patchi].coupled())
        {
            const scalarField& pCDweights = CDweights.boundaryField()[patchi];
            const scalarField& pFaceFlux =
                this->faceFlux_.boundaryField()[patchi];

            const Field<typename Limiter::phiType> plPhiP
            (
                lPhi.boundaryField()[patchi].patchInternalField()
            );
            const Field<typename Limiter::phiType> plPhiN
            (
                lPhi.boundaryField()[patchi].patchNeighbourField()
            );
            const Field<typename Limiter::gradPhiType> pGradcP
            (
                gradc.boundaryField()[patchi].patchInternalField()
            );
            const Field<typename Limiter::gradPhiType> pGradcN
            (
                gradc.boundaryField()[patchi].patchNeighbourField()
            );

            scalarField alphaM
            (
                max
                (
                    alphap.boundaryField()[patchi].patchInternalField(),
                    alphap.boundaryField()[patchi].patchNeighbourField()
                )
            );
            scalarField alphaMinFric
            (
                min
                (
                    alphaMinFriction.boundaryField()[patchi].patchInternalField(),
                    alphaMinFriction.boundaryField()[patchi].patchNeighbourField()
                )
            );
            scalarField alphaMaxi
            (
                min
                (
                    alphaMax.boundaryField()[patchi].patchInternalField(),
                    alphaMax.boundaryField()[patchi].patchNeighbourField()
                )
            );
            scalarField zeta
            (
                max
                (
                    (alphaM - alphaMinFric)/(alphaMaxi - alphaMinFric),
                    0.0
                )
            );
            scalarField G(max(2.0*(1.0 - D_*sqr(zeta)), 0.0));


            // Build the d-vectors
            vectorField pd(CDweights.boundaryField()[patchi].patch().delta());

            forAll(pLim, face)
            {
                pLim[face] = limiter
                (
                    pCDweights[face],
                    pFaceFlux[face],
                    plPhiP[face],
                    plPhiN[face],
                    pGradcP[face],
                    pGradcN[face],
                    pd[face],
                    G[face]
                );
            }
        }
        else
        {
            pLim = 1.0;
        }
    }
}


// * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * * //

template<class Type, class Limiter, template<class> class LimitFunc>
Foam::tmp<Foam::surfaceScalarField>
Foam::KimLimitedScheme<Type, Limiter, LimitFunc>::limiter
(
    const GeometricField<Type, fvPatchField, volMesh>& phi
) const
{
    const fvMesh& mesh = this->mesh();

    const word limiterFieldName(type() + "Limiter(" + phi.name() + ')');

    if (this->mesh().cache("limiter"))
    {
        if (!mesh.foundObject<surfaceScalarField>(limiterFieldName))
        {
            surfaceScalarField* limiterField
            (
                new surfaceScalarField
                (
                    IOobject
                    (
                        limiterFieldName,
                        mesh.time().timeName(),
                        mesh,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh,
                    dimless
                )
            );

            mesh.objectRegistry::store(limiterField);
        }

        surfaceScalarField& limiterField =
            mesh.lookupObjectRef<surfaceScalarField>
            (
                limiterFieldName
            );

        calcLimiter(phi, limiterField);

        return limiterField;
    }
    else
    {
        tmp<surfaceScalarField> tlimiterField
        (
            new surfaceScalarField
            (
                IOobject
                (
                    limiterFieldName,
                    mesh.time().timeName(),
                    mesh
                ),
                mesh,
                dimless
            )
        );

        calcLimiter(phi, tlimiterField.ref());

        return tlimiterField;
    }
}


// ************************************************************************* //
