/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2015-2019 Alberto Passalacqua
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is derivative work of OpenFOAM.

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

#include "univariateODEPopulationBalance.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace PDFTransportModels
{
namespace populationBalanceModels
{
    defineTypeNameAndDebug(univariateODEPopulationBalance, 0);
    addToRunTimeSelectionTable
    (
        ODEPopulationBalanceModel,
        univariateODEPopulationBalance,
        dictionary
    );
}
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::univariateODEPopulationBalance
(
    const word& name,
    const dictionary& dict,
    const surfaceScalarField& phi
)
:
    univariatePDFODETransportModel(name, dict, phi.mesh(), phi, "RPlus"),
    ODEPopulationBalanceModel(name, dict, phi),
    odeType(phi.mesh(), dict),
    aggregation_(dict.lookupOrDefault("aggregation", false)),
    breakup_(dict.lookupOrDefault("breakup", false)),
    growth_(dict.lookupOrDefault("growth", false)),
    nucleation_(dict.lookupOrDefault("nucleation", false)),
    aggregationKernel_(),
    breakupKernel_(),
    growthModel_(),
    diffusionModel_
    (
        Foam::populationBalanceSubModels::diffusionModel::New
        (
            dict.subDict("diffusionModel")
        )
    ),
    nucleationModel_()
{
    if (aggregation_)
    {
        aggregationKernel_ =
            Foam::populationBalanceSubModels::aggregationKernel::New
            (
                dict.subDict("aggregationKernel"),
                phi_.mesh()
            );
    }

    if (breakup_)
    {
        breakupKernel_ =
            Foam::populationBalanceSubModels::breakupKernel::New
            (
                dict.subDict("breakupKernel"),
                phi_.mesh()
            );
    }

    if (growth_)
    {
        growthModel_ =
            Foam::populationBalanceSubModels::growthModel::New
            (
                dict.subDict("growthModel"),
                phi_.mesh()
            );
    }

    if (nucleation_)
    {
        nucleationModel_ =
            Foam::populationBalanceSubModels::nucleationModel::New
            (
                dict.subDict("nucleationModel"),
                phi_.mesh()
            );
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::~univariateODEPopulationBalance()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

Foam::tmp<Foam::fvScalarMatrix>
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::implicitMomentSource
(
    const volScalarMoment& moment
)
{
    return diffusionModel_->momentDiff(moment);
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::updateCellMomentSource(const label)
{}


Foam::scalar
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::cellMomentSource
(
    const labelList& momentOrder,
    const label celli,
    const scalarQuadratureApproximation& quadrature,
    const label environment
)
{
    scalar source = 0.0;

    if (aggregation_)
    {
        source +=
            aggregationKernel_->aggregationSource
            (
                momentOrder,
                celli,
                quadrature,
                environment
            );
    }

    if (breakup_)
    {
        source +=
            breakupKernel_->breakupSource
            (
                momentOrder,
                celli,
                quadrature
            );
    }

    if (growth_)
    {
        source +=
            growthModel_->phaseSpaceConvection
            (
                momentOrder,
                celli,
                quadrature
            );
    }

    if (nucleation_)
    {
        source += nucleationModel_->nucleationSource(momentOrder[0], celli);
    }

    return source;
}


Foam::scalar
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::realizableCo() const
{
    return univariatePDFODETransportModel::realizableCo();
}


Foam::scalar
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::CoNum() const
{
    return 0.0;
}


bool
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::solveMomentSources() const
{
    if (aggregation_ || breakup_ || growth_ || nucleation_)
    {
        return odeType::solveSources_;
    }

    return false;
}


bool
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::solveMomentOde() const
{
    return odeType::solveOde_;
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::explicitMomentSource()
{
    odeType::solve(quadrature_, 0);
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::update()
{
    univariatePDFODETransportModel::update();
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::solve()
{
    univariatePDFODETransportModel::solve();
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::postUpdate()
{
    univariatePDFODETransportModel::postUpdate();
}


void
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::clearODEFields()
{
    univariatePDFODETransportModel::clearODEFields();
}


bool
Foam::PDFTransportModels::populationBalanceModels::univariateODEPopulationBalance
::readIfModified()
{
    odeType::read
    (
        populationBalanceProperties_.subDict(type() + "Coeffs")
    );

    return true;
}


// ************************************************************************* //
