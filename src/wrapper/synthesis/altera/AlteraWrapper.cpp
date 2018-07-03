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
 * @file AlteraWrapper.cpp
 * @brief Implementation of the wrapper to Altera tools
 *
 * Implementation of the methods to wrap synthesis tools by Altera
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
///Header include
#include "AlteraWrapper.hpp"

#include "xml_script_command.hpp"
#include "xml_dom_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "utility.hpp"
#include "constant_strings.hpp"

#include <fstream>

AlteraWrapper::AlteraWrapper(const ParameterConstRef _Param, const std::string& _tool_exec, const target_deviceRef _device, const std::string& _output_dir, const std::string& _default_output_dir) :
   SynthesisTool(_Param, _tool_exec, _device, _output_dir, _default_output_dir)
{
}

AlteraWrapper::~AlteraWrapper()
{

}

void AlteraWrapper::generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name)
{
   // Export reserved (constant) values to design parameters
   for (std::vector<xml_set_variable_tRef>::const_iterator it = xml_reserved_vars.begin(); it != xml_reserved_vars.end(); ++it)
   {
      const xml_set_variable_tRef& var = (*it);
      dp->assign(var->name, getStringValue(var, dp), false);
   }

   // Bare script generation
   std::ostringstream script;
   script << "##########################################################" << std::endl;
   script << "#     Automatically generated by the PandA framework     #" << std::endl;
   script << "##########################################################" << std::endl << std::endl;
   script << generate_bare_script(xml_script_nodes, dp);

   // Replace all reserved variables with their value
   std::string script_string = script.str();
   replace_parameters(dp, script_string);
   /// replace some of the escape sequences
   remove_escaped(script_string);

   // Save the generated script
   if (boost::filesystem::exists(file_name)) boost::filesystem::remove_all(file_name);
   script_name = file_name;
   std::ofstream file_stream(file_name.c_str());
   file_stream << script_string << std::endl;
   file_stream.close();
}

std::string AlteraWrapper::getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const
{
   switch(node->nodeType)
   {
      case NODE_VARIABLE:
      {
         std::string result;
         const xml_set_variable_t* var = GetPointer<xml_set_variable_t>(node);
         if (var->singleValue)
            result += *(var->singleValue);
         else if (var->multiValues.size())
         {
            result += "{";
            for (std::vector<xml_set_entry_tRef>::const_iterator it = var->multiValues.begin(); it != var->multiValues.end(); ++it)
            {
               const xml_set_entry_tRef e = *it;
               if (it != var->multiValues.begin())
                  result += " ";
               result += toString(e, dp);
            }
            result += "}";
         }
         else
            result += "\"\"";
         return result;
      }
      case NODE_COMMAND:
      case NODE_ENTRY:
      case NODE_FOREACH:
      case NODE_ITE_BLOCK:
      case NODE_PARAMETER:
      case NODE_SHELL:
      case NODE_UNKNOWN:
      default:
         THROW_ERROR("Not supported node type: " + STR(node->nodeType));
   }
   ///this point should never be reached
   return "";
}

std::string AlteraWrapper::toString(const xml_script_node_tRef node, const DesignParametersRef dp) const
{
   switch(node->nodeType)
   {
      case NODE_ENTRY:
      {
         const xml_set_entry_t* ent = GetPointer<xml_set_entry_t>(node);
         return ent->value;
      }
      case NODE_VARIABLE:
      {
         const xml_set_variable_t* var = GetPointer<xml_set_variable_t>(node);
         return "set " + var->name + " " + getStringValue(node, dp) + ";";
      }
      case NODE_PARAMETER:
      {
         const xml_parameter_t* par = GetPointer<xml_parameter_t>(node);
         std::string result;
         if (par->name)
            result += *(par->name);
         if (par->name && (par->singleValue || par->multiValues.size()))
            result += par->separator;
         if (par->singleValue)
            result += *(par->singleValue);
         else if (par->multiValues.size())
         {
            result += par->curlyBrackets ? "{" : "\"";
            for (std::vector<xml_set_entry_tRef>::const_iterator it = par->multiValues.begin(); it != par->multiValues.end(); ++it)
            {
               const xml_set_entry_tRef p = *it;
               if (it != par->multiValues.begin())
                  result += " ";
               result += toString(p, dp);
            }
            result += par->curlyBrackets ? "}" : "\"";
         }
         return result;
      }
      case NODE_COMMAND:
      {
         const xml_command_t* comm = GetPointer<xml_command_t>(node);
         // TODO: Evaluate the condition
         std::string result;
         if (comm->name)
            result += *(comm->name);
         if (comm->name && comm->value)
            result += " ";
         if (comm->value)
            result += *(comm->value);
         if (comm->parameters.size())
            for (std::vector<xml_parameter_tRef>::const_iterator it = comm->parameters.begin(); it != comm->parameters.end(); ++it)
            {
               const xml_parameter_tRef p = *it;
               result += " " + toString(p, dp);
            }
         if (comm->output)
         {
            result += " >> " + *(comm->output);
         }
         return result;
      }
      case NODE_SHELL:
      {
         const xml_shell_t* sh = GetPointer<xml_shell_t>(node);
         // TODO: Evaluate the condition
         std::string result = "sh ";
         if (sh->name)
            result += *(sh->name);
         if (sh->name && sh->value)
            result += " ";
         if (sh->value)
            result += *(sh->value);
         if (sh->parameters.size())
            for (std::vector<xml_parameter_tRef>::const_iterator it = sh->parameters.begin(); it != sh->parameters.end(); ++it)
            {
               const xml_parameter_tRef p = *it;
               result += " " + toString(p, dp);
            }
         if (sh->output)
         {
            result += " >> " + *(sh->output);
         }
         return result;
      }
      case NODE_ITE_BLOCK:
      {
         const xml_ite_block_t* ite = GetPointer<xml_ite_block_t>(node);
         std::string result;
         bool conditionValue = ite->evaluate_condition(&(ite->condition), dp), first = true;
         const std::vector<xml_script_node_tRef>& block = conditionValue ? ite->thenNodes : ite->elseNodes;
         for (std::vector<xml_script_node_tRef>::const_iterator it = block.begin(); it != block.end(); ++it)
         {
            const xml_script_node_tRef n = *it;
            if (n->checkCondition(dp))
            {
               if (!first)
                  result += "\n";
               first = false;
               result += toString(n, dp);
            }
         }
         return result;
      }
      case NODE_UNKNOWN:
      case NODE_FOREACH:
      default:
         THROW_ERROR("Not supported node type: " + STR(node->nodeType));
   }
   ///this point should never be reached
   return "";
}

