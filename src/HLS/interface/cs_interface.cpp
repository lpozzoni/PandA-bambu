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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @file cs_interface.cpp
 * @brief Class to generate the interface for the context switch project
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
*/

#include "cs_interface.hpp"
#include "memory.hpp"
#include "memory_cs.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "BambuParameter.hpp"
#include "hls_manager.hpp"
#include "structural_manager.hpp"
#include "hls.hpp"
#include "copyrights_strings.hpp"
#include "hls_target.hpp"

cs_interface::cs_interface(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type) :
   module_interface(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   THROW_ASSERT(funId, "Function not set in minimal interface");
}

cs_interface::~cs_interface()
{

}

DesignFlowStep_Status cs_interface::InternalExec()
{
   const structural_managerRef SM = HLS->top;
   if (!SM) THROW_ERROR("Top component has not been created yet!");

   structural_objectRef wrappedObj = SM->get_circ();
   std::string module_name = wrappedObj->get_id() + "_cs_interface";

   structural_managerRef SM_cs_interface =structural_managerRef(new structural_manager(parameters));
   structural_type_descriptorRef module_type =structural_type_descriptorRef(new structural_type_descriptor(module_name));
   SM_cs_interface->set_top_info(module_name, module_type);
   structural_objectRef interfaceObj = SM_cs_interface->get_circ();

   // add the core to the wrapper
   wrappedObj->set_owner(interfaceObj);
   wrappedObj->set_id(wrappedObj->get_id() + "_i0");
   GetPointer<module>(interfaceObj)->add_internal_object(wrappedObj);
   ///Set some descriptions and legal stuff
   GetPointer<module>(interfaceObj)->set_description("Minimal interface for top component: " + wrappedObj->get_typeRef()->id_type);
   GetPointer<module>(interfaceObj)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(interfaceObj)->set_authors("Component automatically generated by bambu");
   GetPointer<module>(interfaceObj)->set_license(GENERATED_LICENSE);

   add_parameter_port(SM_cs_interface,interfaceObj,wrappedObj);   //connect port
   structural_objectRef clock_port, reset_port;
   structural_objectRef port_ck = wrappedObj->find_member(CLOCK_PORT_NAME, port_o_K, wrappedObj);
   clock_port= SM_cs_interface->add_port(GetPointer<port_o>(port_ck)->get_id(), port_o::IN,
                                              interfaceObj, port_ck->get_typeRef());
   SM_cs_interface->add_connection(clock_port, port_ck);

   structural_objectRef port_rst = wrappedObj->find_member(RESET_PORT_NAME, port_o_K, wrappedObj);
   reset_port= SM_cs_interface->add_port(GetPointer<port_o>(port_rst)->get_id(), port_o::IN,
                                              interfaceObj, port_rst->get_typeRef());
   SM_cs_interface->add_connection(reset_port, port_rst);

   instantiate_component_parallel(SM_cs_interface,clock_port,reset_port);   //instantiate memory_ctrl_top

   manage_extern_global_port_top(SM_cs_interface,wrappedObj,interfaceObj);  //connect memory port for memory_ctrl_top

   memory::propagate_memory_parameters(HLS->top->get_circ(), SM_cs_interface);
   // Generation completed, the new created module substitutes the current top-level one
   HLS->top = SM_cs_interface;
   return DesignFlowStep_Status::SUCCESS;
}

void cs_interface::add_parameter_port(const structural_managerRef SM, structural_objectRef circuit, structural_objectRef top_module)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting parameter ports");
   for(unsigned int j = 0; j < GetPointer<module>(top_module)->get_in_port_size(); j++)   //connect in
   {
      structural_objectRef port_i = GetPointer<module>(top_module)->get_in_port(j);
      if(!GetPointer<port_o>(port_i)->get_is_memory())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         if(port_name!=CLOCK_PORT_NAME && port_name!=RESET_PORT_NAME)
         {
            structural_objectRef port_obj = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
            SM->add_connection(port_obj,port_i);
         }
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(top_module)->get_out_port_size(); j++) //connect out
   {
      structural_objectRef port_i = GetPointer<module>(top_module)->get_out_port(j);
      if(!GetPointer<port_o>(port_i)->get_is_memory())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         structural_objectRef port_obj = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
         SM->add_connection(port_i,port_obj);
      }
   }
}

void cs_interface::instantiate_component_parallel(const structural_managerRef SM, structural_objectRef clock_port, structural_objectRef reset_port)
{
   const structural_objectRef circuit = SM->get_circ();
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Start to instantiate memory_ctrl_top");
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   std::string memory_ctrl_model;
   if(HLS->Param->getOption<unsigned int>(OPT_channels_number)!=1)
      memory_ctrl_model = "memory_ctrl";
   else
      memory_ctrl_model = "memory_ctrl_sigle_input";
   std::string memory_ctrl_name = "memory_ctrl_top";
   std::string memory_ctrl_library = HLS->HLS_T->get_technology_manager()->get_library(memory_ctrl_model);
   structural_objectRef mem_ctrl_mod = SM->add_module_from_technology_library(memory_ctrl_name, memory_ctrl_model, memory_ctrl_library, circuit, HLS->HLS_T->get_technology_manager());

   structural_objectRef clock_mem_ctrl = mem_ctrl_mod->find_member(CLOCK_PORT_NAME,port_o_K,mem_ctrl_mod);
   structural_objectRef clock_sign=SM->add_sign("clock_mem_ctrl_signal", circuit, bool_type);
   SM->add_connection(clock_sign, clock_port);
   SM->add_connection(clock_sign, clock_mem_ctrl);

   structural_objectRef reset_mem_ctrl = mem_ctrl_mod->find_member(RESET_PORT_NAME,port_o_K,mem_ctrl_mod);
   structural_objectRef reset_sign=SM->add_sign("reset_mem_ctrl_signal", circuit, bool_type);
   SM->add_connection(reset_sign, reset_port);
   SM->add_connection(reset_sign, reset_mem_ctrl);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Instantiate memory_ctrl_top!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting setting parameter memory_ctrl_top!");
   GetPointer<module>(mem_ctrl_mod)->SetParameter("NUM_CHANNEL", STR(parameters->getOption<unsigned int>(OPT_channels_number)));
   GetPointer<module>(mem_ctrl_mod)->SetParameter("NUM_BANK", STR(parameters->getOption<unsigned int>(OPT_memory_banks_number)));
   unsigned int addr_task=static_cast<unsigned int>(log2(parameters->getOption<unsigned int>(OPT_context_switch)));
   if(!addr_task) addr_task=1;
   GetPointer<module>(mem_ctrl_mod)->SetParameter("ADDR_TASKS", STR(addr_task));
   unsigned int addr_kern=static_cast<unsigned int>(log2(parameters->getOption<unsigned int>(OPT_num_threads)));
   if(!addr_kern) addr_kern=1;
   GetPointer<module>(mem_ctrl_mod)->SetParameter("ADDR_ACC", STR(addr_kern));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter memory_ctrl_top setted!");

   resize_memory_ctrl_ports(mem_ctrl_mod);
}

void cs_interface::resize_memory_ctrl_ports(structural_objectRef mem_ctrl_mod)
{
   unsigned int memory_channel=HLS->Param->getOption<unsigned int>(OPT_channels_number);
   unsigned int num_banks=HLS->Param->getOption<unsigned int>(OPT_memory_banks_number);
   for(unsigned int j = 0; j < GetPointer<module>(mem_ctrl_mod)->get_in_port_size(); j++)  //resize input port
   {
      structural_objectRef port_i = GetPointer<module>(mem_ctrl_mod)->get_in_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         if(port_name.substr(0,3)=="IN_")
            resize_dimension_bus_port(num_banks, port_i);
         else
            resize_dimension_bus_port(memory_channel, port_i);
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(mem_ctrl_mod)->get_out_port_size(); j++)    //resize output port
   {
      structural_objectRef port_i = GetPointer<module>(mem_ctrl_mod)->get_out_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         if(port_name.substr(0,4)=="OUT_")
            resize_dimension_bus_port(num_banks, port_i);
         else
            resize_dimension_bus_port(memory_channel, port_i);
      }
   }
}

void cs_interface::resize_dimension_bus_port(unsigned int vector_size, structural_objectRef port)
{
   unsigned int bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   unsigned int bus_addr_bitsize = HLSMgr->Rmem->get_bus_addr_bitsize();
   unsigned int bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   unsigned int bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();

   if (GetPointer<port_o>(port)->get_is_data_bus())
      port->type_resize(bus_data_bitsize);
   else if (GetPointer<port_o>(port)->get_is_addr_bus())
      port->type_resize(bus_addr_bitsize);
   else if (GetPointer<port_o>(port)->get_is_size_bus())
      port->type_resize(bus_size_bitsize);
   else if (GetPointer<port_o>(port)->get_is_tag_bus())
      port->type_resize(bus_tag_bitsize);

   GetPointer<port_o>(port)->add_n_ports(vector_size,port);
}

void cs_interface::manage_extern_global_port_top(const structural_managerRef SM, const structural_objectRef memory_module, const structural_objectRef circuit)
{
   structural_objectRef cir_port;
   structural_objectRef memory_ctrl_port;
   unsigned int num_channel=HLS->Param->getOption<unsigned int>(OPT_channels_number);
   structural_objectRef memory_ctrl = circuit->find_member("memory_ctrl_top", component_o_K, circuit);
   THROW_ASSERT(memory_ctrl, "NULL, memmory_ctrl");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting memory_port of memory_ctrl");
   for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_in_port_size(); j++)  //from memory_ctrl output to module input
   {
      structural_objectRef port_i = GetPointer<module>(memory_module)->get_in_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory() && GetPointer<port_o>(port_i)->get_is_global() && GetPointer<port_o>(port_i)->get_is_extern())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         memory_ctrl_port = memory_ctrl->find_member(port_name, port_vector_o_K, memory_ctrl);
         structural_objectRef memory_Sign=SM->add_sign_vector(port_name+"_signal", num_channel, circuit, port_i->get_typeRef());
         THROW_ASSERT(!memory_ctrl_port || GetPointer<port_o>(memory_ctrl_port), "should be a port");
         SM->add_connection(memory_ctrl_port, memory_Sign);
         SM->add_connection(memory_Sign, port_i);
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(memory_module)->get_out_port_size(); j++)    //from module output to memory_ctrl input
   {
      structural_objectRef port_i = GetPointer<module>(memory_module)->get_out_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory() && !GetPointer<port_o>(port_i)->get_is_global() && !GetPointer<port_o>(port_i)->get_is_extern())
      {
         std::string port_name = GetPointer<port_o>(port_i)->get_id();
         memory_ctrl_port = memory_ctrl->find_member(port_name, port_vector_o_K, memory_ctrl);
         structural_objectRef memory_Sign=SM->add_sign_vector(port_name+"_signal", num_channel, circuit, port_i->get_typeRef());
         THROW_ASSERT(!memory_ctrl_port || GetPointer<port_o>(memory_ctrl_port), "should be a port");
         SM->add_connection(port_i, memory_Sign);
         SM->add_connection(memory_Sign, memory_ctrl_port);
      }
   }

   for(unsigned int j = 0; j < GetPointer<module>(memory_ctrl)->get_in_port_size(); j++)  //connect input memory_ctrl with input circuit
   {
      structural_objectRef port_i = GetPointer<module>(memory_ctrl)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      if(GetPointer<port_o>(port_i)->get_is_memory() && GetPointer<port_o>(port_i)->get_is_global() && GetPointer<port_o>(port_i)->get_is_extern() && port_name.substr(0,3)=="IN_")
      {
         cir_port = circuit->find_member(port_name.erase(0,3), port_i->get_kind(), circuit);
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
         }
         SM->add_connection(cir_port,port_i);
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(memory_ctrl)->get_out_port_size(); j++)    //connect output memory_ctrl with output circuit
   {
      structural_objectRef port_i = GetPointer<module>(memory_ctrl)->get_out_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      if(GetPointer<port_o>(port_i)->get_is_memory() && !GetPointer<port_o>(port_i)->get_is_global() && !GetPointer<port_o>(port_i)->get_is_extern() && port_name.substr(0,4)=="OUT_")
      {
         cir_port = circuit->find_member(port_name.erase(0,4), port_i->get_kind(), circuit); //delete OUT from port name
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
         }
         SM->add_connection(cir_port,port_i);
      }
   }
}
