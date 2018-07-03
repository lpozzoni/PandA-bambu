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
 * @file unique_binding_register.cpp
 * @brief Class implementation of unique binding register allocation algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @version $Revision$
 * @date $Date$

*/
#include "unique_binding_register.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "reg_binding.hpp"
#include "liveness.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "boost/lexical_cast.hpp"

///HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

///tree include
#include "behavioral_helper.hpp"

///utility include
#include "cpu_time.hpp"

unique_binding_register::unique_binding_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager) :
   reg_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::UNIQUE_REGISTER_BINDING)
{

}

unique_binding_register::~unique_binding_register()
{

}

DesignFlowStep_Status unique_binding_register::InternalExec()
{
   long step_time;
   START_TIME(step_time);
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");
   HLS->Rreg = reg_bindingRef(new reg_binding(HLS, HLSMgr));
   const std::list<vertex> & support = HLS->Rliv->get_support();

   const std::list<vertex>::const_iterator vEnd = support.end();
   for(std::list<vertex>::const_iterator vIt = support.begin(); vIt != vEnd; ++vIt)
   {
      const std::set<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
      std::set<unsigned int>::iterator k_end = live.end();
      for(std::set<unsigned int>::iterator k = live.begin(); k != k_end; ++k)
      {
         unsigned int storage_value_index = HLS->storage_value_information->get_storage_value_index(*vIt, *k);
         HLS->Rreg->bind(storage_value_index, storage_value_index);
      }
   }
   HLS->Rreg->set_used_regs(HLS->storage_value_information->get_number_of_storage_values());
   STOP_TIME(step_time);
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "-->Register binding information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   if (output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      HLS->Rreg->print();
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform register binding: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "<--");
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
