/*==============================================================================
 * Public Domain Contributions 2009 United States Government                   *
 * as represented by the U.S. Air Force Research Laboratory.                   *
 * Copyright (C) 2006, 2008 Spencer E. Olson                                   *
 *                                                                             *
 * This file is part of CHIMP                                                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify it     *
 * under the terms of the GNU Lesser General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or (at your  *
 * option) any later version.                                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public        *
 * License for more details.                                                   *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 -----------------------------------------------------------------------------*/



#include <chimp/RuntimeDB.h>
#include <chimp/interaction/filter/Or.h>
#include <chimp/interaction/filter/Elastic.h>
#include <chimp/interaction/filter/Label.h>
#include <chimp/interaction/v_rel_fnc.h>

#include <xylose/upper_triangle.h>
#include <xylose/random/Kiss.hpp>
#include <xylose/distribution/Inverter.h>
#include <xylose/distribution/Gaussian.h>

#include <physical/physical.h>

#include <iostream>
#include <iterator>
#include <cmath>
#include <cassert>


using physical::unit::nm;      /**< nanometer */
using physical::unit::K;       /**< Kelvin */
using physical::constant::si::K_B; /**< Boltzmann constant */
static const double nm2 = nm*nm;

/** The temperature of the entire population. */
const static double temperature = 100 * K;

typedef xylose::random::Kiss RNG;

int main() {
  namespace filter = chimp::interaction::filter;
  typedef boost::shared_ptr<filter::Base> SP;
  typedef chimp::RuntimeDB<> DB;

  DB db;
  RNG rng;

  /* load information from XML file. */
  db.addParticleType("87Rb");
  db.addParticleType("Ar"  );
  db.addParticleType("e^-" );
  db.addParticleType("Hg"  );
  db.addParticleType("Hg^+");

  db.filter = SP(
                new filter::Or( SP(new filter::Elastic),
                                SP(new filter::Label("inelastic")) )
              );

  /* set up the the runtime database */
  db.initBinaryInteractions();


  /* *** BEGIN Set up data (single and cross species). ***
   * The velocity and maxSigmaVelProduct containers here are just intended to
   * demonstrate the kind of information that would be required for extensive
   * use of the chimp library and information that would typically be required
   * per cell in a gridded type of simulation. */
  /** velocity distributions. */
  std::vector< xylose::distribution::Inverter<> > velocity;
  /** max(sigma * v_rel). */
  xylose::upper_triangle<double> maxSigmaVelProduct;
  {
    namespace dist = xylose::distribution;
    using dist::Inverter;
    using dist::Gaussian;
    typedef DB::PropertiesVector::const_iterator PIter;
    using chimp::property::mass;
    using chimp::interaction::estMaxVFromStdV;
    using chimp::interaction::calcStdVFromT;

    PIter end = db.getProps().end();
    for ( PIter i  = db.getProps().begin(); i != end; ++i ) {

      /* set the velocity distribution for this particle type using the global
       * temperature. */
      double beta = 0.5 * i->mass::value / (K_B * temperature);
      double sigma = std::sqrt( 0.5 / beta );
      velocity.push_back( Inverter<>( Gaussian(beta), -4*sigma, 4*sigma ) );

      /* now set cross species data for i */
      for ( PIter j  = i; j != end; ++j ) {
        double reduced_mass =   i->mass::value * j->mass::value
                            / ( i->mass::value + j->mass::value);
        // FIXME:  I forgot to call db(i,j).cs->findMaxSigmaVProduct( max_v )
        maxSigmaVelProduct.push_back(
          estMaxVFromStdV( calcStdVFromT( temperature, reduced_mass ) )
        );
      }
    }

    int n = db.getProps().size();
    assert( maxSigmaVelProduct.size() == static_cast<unsigned int>(n*(n+1)/2) );
    maxSigmaVelProduct.resize(n);
    assert( maxSigmaVelProduct.edge_size() == n );
  }
  /* *** END   Set up data (single and cross species). *** */


  /* spit out known interactions and attempt execution */
  std::cout << "\n\nSmall calculateOutPath test for each interaction set "
               "known to the runtime database:\n";
  typedef DB::InteractionTable::const_iterator CIter;
  for ( CIter i  = db.getInteractions().begin();
              i != db.getInteractions().end(); ++i ) {
    const int & A = i->lhs.A.species;
    const int & B = i->lhs.B.species;

    if (i->rhs.size() == 0)
      /* no interactions for these inputs. */
      continue;

    /* for this test, we'll just pick the velocity randomly from the Maxwellian
     * distribution. */
    double v_rel = std::abs( velocity[A]() - velocity[B]() );

    double & m_s_v = maxSigmaVelProduct(A,B);

    /* This calculates the proper output path, observing both absolute
     * probability that any interaction occurs as well as relative probabilities
     * for when an interaction does occur.  
     *
     * The .first<int> component is the interaction index, such as
     * i->rhs[path.first] to obtain the Equation instance for the interaction
     * that should occur.  
     * The .second<double> component is the cross section value of this
     * particular interaction for the given relative velocity.   */
    std::pair<int,double> path = i->calculateOutPath(m_s_v, v_rel, rng);

    i->print(std::cout << "Performing test interactions for:", db) << '\n';
    if ( path.first < 0 )
      std::cout << "\tNo interaction occurred\n";
    else {
        i->rhs[path.first].print(
            std::cout << "Interaction occurred on path:  ", db ) << "\n"
                         "-----\tmax_sigma_v  :  " << m_s_v << "\n"
                         "-----\tv_rel        :  " << v_rel << "\n"
                         "-----\tcross section:  " << (path.second/nm2) << " nm^2\n";

    }
  }

  std::cout << std::endl;

  return 0;
}
