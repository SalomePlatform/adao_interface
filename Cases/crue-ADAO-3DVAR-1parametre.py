# -*- coding: utf-8 -*-
# Copyright (C) 2019 - Jean-Philippe Argaud

import numpy as np
import unittest
np.set_printoptions(precision=3)
from adao import adaoBuilder

# Les entrées
nbobs = 4
Qobs = np.array([10.,20.,30.,40.])
Hobs = np.array([0.19694513, 0.298513, 0.38073079, 0.45246109])

# Définit les paramètres de l'AD
KsInitial = 20.
ZvInitial = 49.
ZmInitial = 51.
thetaB = [KsInitial,]

def functionCrue(Q, K_s):
    L = 5.0e3
    B = 300.0
    Z_v = ZvInitial
    Z_m = ZmInitial
    alpha = (Z_m - Z_v)/L
    H = (Q/(K_s*B*np.sqrt(alpha)))**(3.0/5.0)
    return H

def obsFunction(theta):
    # Evaluation de la sortie du modèle
    K_s = float(theta)
    sortie = np.zeros((nbobs,))
    for i in range(nbobs):
        sortie[i] = functionCrue(Qobs[i],K_s)
    return sortie

def obsFunctionMulti(thetas):
    ret = []
    for elt in thetas:
        ret.append( np.array(obsFunction(elt)) )
    return ret

class TestRefFloodTest(unittest.TestCase):

    def test0(self):
        # define the problem bounds
        boundsMin = [20.,]
        boundsMax = [40.,]

        # Ecart-type des paramètres
        sigmaKs = 5.e10
        sigmaZv = 1.
        sigmaZm = 1.
        sigmaTheta = [sigmaKs,]

        # Ecart-type des observations
        sigmaH = 0.5 # (m^2)

        case = adaoBuilder.New()
        case.set( 'AlgorithmParameters', Algorithm='3DVAR' )
        case.set( 'Background',          Vector=thetaB)
        case.set( 'BackgroundError',     DiagonalSparseMatrix=sigmaTheta )
        case.set( 'Observation',         Vector=Hobs)
        case.set( 'ObservationError',    ScalarSparseMatrix=sigmaH )
        case.set( 'ObservationOperator', OneFunction= obsFunctionMulti, InputFunctionAsMulti = True)
        case.execute()
        res = case.get("Analysis")[-1][0]
        self.assertAlmostEqual(res,25.,6)
        pass
    pass

if __name__ == "__main__":
    unittest.main()
    pass

