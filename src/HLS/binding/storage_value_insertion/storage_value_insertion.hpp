/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project 
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (c) 2004-2018 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
/**
 * @file storage_value_insertion.hpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @defgroup Storage Value Insertion
 * @ingroup HLS
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/
#ifndef STORAGE_VALUE_INSERTION_HPP
#define STORAGE_VALUE_INSERTION_HPP

#include "config_HAVE_EXPERIMENTAL.hpp"

#include "hls_function_step.hpp"
REF_FORWARD_DECL(storage_value_insertion);

#include "graph.hpp"

class storage_value_insertion : public HLSFunctionStep
{
   protected:
      /**
       * Compute the relationship of this step
       * @param relationship_type is the type of relationship to be considered
       * @return the steps in relationship with this
       */
      virtual const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship> > ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   public:
      /**
       * Constructor
       * @param design_flow_manager is the design flow manager
       * @param hls_flow_step_type is the type of storage value insertion algorithm
       */
      storage_value_insertion(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

      /**
       * Destructor.
       */
      virtual ~storage_value_insertion();

      /**
       * Initialize the step (i.e., like a constructor, but executed just before exec
       */
      virtual void Initialize();
};
#endif
