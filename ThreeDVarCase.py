# coding: utf-8
#
# Copyright (C) 2019 EDF R&D
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#
# Author: Anthony Geay, anthony.geay@edf.fr, EDF R&D

# ***** <class 'numpy.matrixlib.defmatrix.matrix'>
# ***** <class 'numpy.ndarray'>
# ***** <class 'numpy.ndarray'>
# ***** <class 'numpy.ndarray'>

def BuildCase(cppFunc):
    def evaluator( xserie ):
        import numpy as np
        yserie = [np.array(elt) for elt in cppFunc(xserie)]
        return yserie
    
    from adao import adaoBuilder
    #
    Xb = (5.0, 7, 9.0)
    observations = [2, 6, 12, 20]
    alphamin, alphamax = 0., 10.
    betamin,  betamax  = 3, 13
    gammamin, gammamax = 1.5, 15.5
    Bounds = (
        (alphamin, alphamax),
        (betamin,  betamax ),
        (gammamin, gammamax))
    #
    # TUI ADAO
    # --------
    case = adaoBuilder.New()
    case.set( 'AlgorithmParameters',
        Algorithm = '3DVAR',                  # Mots-clé réservé
        Parameters = {                        # Dictionnaire
            "Bounds":Bounds,                  # Liste de paires de Real ou de None
            "MaximumNumberOfSteps":100,       # Int >= 0
            "CostDecrementTolerance":1.e-7,   # Real > 0
            "StoreSupplementaryCalculations":[# Liste de mots-clés réservés
                "CostFunctionJAtCurrentOptimum",
                "CostFunctionJoAtCurrentOptimum",
                "CurrentOptimum",
                "SimulatedObservationAtCurrentOptimum",
                "SimulatedObservationAtOptimum",
                ],
            }
        )
    case.set( 'Background',
        Vector = Xb,                          # array, list, tuple, matrix
        Stored = True,                        # Bool
        )
    case.set( 'Observation',
        Vector = observations,                # array, list, tuple, matrix
        Stored = False,                       # Bool
        )
    case.set( 'BackgroundError',
        Matrix = None,                        # None ou matrice carrée
        ScalarSparseMatrix = 1.0e10,          # None ou Real > 0
        DiagonalSparseMatrix = None,          # None ou vecteur
        )
    case.set( 'ObservationError',
        Matrix = None,                        # None ou matrice carrée
        ScalarSparseMatrix = 1.0,             # None ou Real > 0
        DiagonalSparseMatrix = None,          # None ou vecteur
        )
    case.set( 'ObservationOperator',
        OneFunction = evaluator,              # MultiFonction [Y] = F([X]) multisimulation
        Parameters  = {                       # Dictionnaire
            "DifferentialIncrement":0.0001,   # Real > 0
            "CenteredFiniteDifference":False, # Bool
            },
        InputFunctionAsMulti = True,          # Bool
        )
    case.set( 'Observer',
        Variable = "CurrentState",            # Mot-clé
        Template = "ValuePrinter",            # Mot-clé
        String   = None,                      # None ou code Python
        Info     = None,                      # None ou string
        )
    return case
