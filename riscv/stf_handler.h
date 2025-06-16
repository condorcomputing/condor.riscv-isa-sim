// --------------------------------------------------------------------------
// Copyright (C) 2024, Jeff Nye, Condor Computing
//
// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// --------------------------------------------------------------------------
#pragma once
#include "cfg.h"
#include "decode.h"
#include "decode_macros.h"
#include "encoding.h"
#include "fesvr/option_parser.h"
#include "platform.h"
#include "sim.h"

#include "git-version.h"

#include "stf-inc/stf_record_types.hpp"
#include "stf-inc/stf_writer.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std::chrono;
//using namespace std; //FIXME

#define EN_LOGGING 0
#if EN_LOGGING == 1
#define LOG(s) std::cout<<s<<std::endl;
#else
#define LOG(s)
#endif
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
struct stf_mem_access_t
{
  stf_mem_access_t(uint64_t _vaddr, uint64_t _size, uint64_t _value)
    : vaddr(_vaddr), size(_size), value(_value)
  {}

  uint64_t vaddr;
  uint64_t size;
  uint64_t value;
};
// --------------------------------------------------------------------------
// Keep in case more fields are needed
// --------------------------------------------------------------------------
struct stf_reg_access_t
{
  stf_reg_access_t(uint64_t _idx)
    : idx(_idx)
  {}
  uint64_t idx;
};

class stf_trace_complete: public std::runtime_error {
public:
    explicit stf_trace_complete(const std::string &what)
            : std::runtime_error(what) {}
};
// ==========================================================================
// Instruction tracing, STF format
//
// TODO: description
// TODO: at the moment tracing is supported when 1 processor is begin simulated.
// ========================================================================== 
struct StfHandler
{
  // ----------------------------------------------------------------
  // singleton 
  // ----------------------------------------------------------------
  static StfHandler* getInstance() {
    if(!instance) instance = new StfHandler();
    return instance;
  }
  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  inline void initialize_if(processor_t *p,insn_fetch_t &fetch) {
    //stf_writer will for the most part be initialized except at start
    if((bool)stf_writer == false && in_traceable_region()) [[unlikely]] {
      open_trace(p,fetch);
    }
  }
  // ---------------------------------------------------------------- 
  //Do any required cleanup before forcing exit, nothing currently
  // ---------------------------------------------------------------- 
  void terminate_simulator() {
    throw stf_trace_complete("STF trace capture complete.\n");
  }
  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  bool stf_writer_enabled() { return (bool)stf_writer != false; }
  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  void report_stats(sim_t &s,cfg_t &cfg,
       time_point<high_resolution_clock> &start)
  {
    std::vector<long int> bbv_insns_per_core;
    std::vector<uint64_t> bbv_insn_roi_start_per_core;
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start).count();
    double duration_ms = static_cast<double>(duration);

    uint64_t instret_count = 0;
    for(size_t idx=0;idx<cfg.nprocs();++idx) {
      auto instret_count_n = s.get_core(idx)->get_state()->minstret->read();
      instret_count += instret_count_n;
    }

    double mips= 0;
    if(duration_ms > 0) {
      mips = ((instret_count / duration_ms ) * (1000.)) / 1000000.;
    }

    if(!s.in_quiet_mode()) {
      fprintf(stderr,"-I: Totals: \n");
      fprintf(stderr,"-I:   traced instructions         %ld\n",
                            _traced_instructions_running);
      fprintf(stderr,"-I:   executed instructions       %ld\n",
                            executed_instructions);
      fprintf(stderr,"-I:   instructions retired        %ld\n",  instret_count);
      fprintf(stderr,"-I:   duration ms                 %.2f\n", duration_ms);
      fprintf(stderr,"-I:   wall-clock MIPs             %.2f\n", mips);

      for(size_t idx=0;idx<cfg.nprocs();++idx) {
        auto bbv_num_insns = s.get_core(idx)->get_bb_tracer().get_total_insns();
        fprintf(stderr,"-I:   BBV instructions, core %ld    %ld\n", idx, bbv_num_insns);
        bbv_insns_per_core.push_back(bbv_num_insns);
      }
      for(size_t idx=0;idx<cfg.nprocs();++idx) {
        auto bbv_insn_count_roi_start = s.get_core(idx)->get_bb_tracer().get_insn_count_on_roi_start();
        fprintf(stderr,"-I:   BBV ROI start insn, core %ld  %ld\n", idx, bbv_insn_count_roi_start);
        bbv_insn_roi_start_per_core.push_back(bbv_insn_count_roi_start);
      }
    }

    write_json_stats(_traced_instructions_running,
                     executed_instructions, instret_count,
                     duration_ms, mips, bbv_insns_per_core, bbv_insn_roi_start_per_core,
                     _traced_warmup_insns);
  }
  // ---------------------------------------------------------------- 
  // Getters
  // ---------------------------------------------------------------- 
  bool trace_memory_records()    {  return _trace_memory_records;   }
  bool trace_register_state()    {  return _trace_register_state;   }
  bool stf_enable_log_commits()  {  return _trace_memory_records
                                        || _trace_register_state;   }
  bool traced_instructions_running() {  return _traced_instructions_running; }
  bool traced_instructions_region()  {  return _traced_instructions_region;  }
  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  void update_state(processor_t *proc,uint32_t bits,reg_t pc,reg_t npc)
  {
    last_npc = npc;
    is_taken_branch = proc->get_state()->taken_branch_flag;

    if(pc != npc && npc != PC_SERIALIZE_BEFORE && npc != PC_SERIALIZE_AFTER) {
      insn_bytes = (bits & 0x3) == 0x3 ? 4 : 2;
      if (npc != pc + insn_bytes) {
        if (!is_taken_branch) {
           std::cerr << "UNDETECTED BRANCH from 0x" << std::hex << pc << " to 0x" << npc << std::endl;
        }
        assert(is_taken_branch);
      }
    }
  }
  // ---------------------------------------------------------------- 
  // Common trace instruction method which selects specific trace
  // method based on trace mode
  // ---------------------------------------------------------------- 
  void trace_insn(processor_t *p,insn_fetch_t &fetch,
                  reg_t pc, reg_t npc, std::string debug="")
  {

    //tracing is not being used
    if(!macro_tracing && !insn_num_tracing) {
      LOG("-trace_insn "+debug+" NOTHING SELECTED");
      return;
    }

    update_state(p,fetch.insn.bits(),pc,npc);

    //This is always cleared, set in transistion from fast to slow
    _pending_region = false; 

    if(macro_tracing) trace_macro_insn(p,fetch,debug);
    else              trace_count_insn(p,fetch,debug);
  }

  // ---------------------------------------------------------------- 
  // If this is called then macro_tracing == false
  // ---------------------------------------------------------------- 
  void trace_count_insn(processor_t *proc,insn_fetch_t &fetch,
                        std::string debug="")
  {
    _pending_region = false;
    auto const state = proc->get_state();
    auto const PC = state->pc;

    bool stop_a =  is_stop_macro(fetch.insn.bits()) && exit_on_stop_opc;
    bool stop_b =  insn_count == UINT64_MAX ? false : executed_instructions >= insn_start+insn_count;


    if(unlikely(stop_a)) {
      info(proc,"trace stop  opc detected 0x%lx\n",PC);
    } 

    if(unlikely(stop_b)) {
      info(proc,"tracing stop insn count reached at PC:0x%lx\n",PC);
    }

    if(unlikely(stop_a || stop_b)) {
      _in_trace_region = false; //redundant, for clarity/debug
      report_stats(proc,debug);
      close_trace();
      if(stop_a) {
        info(proc,"--stf_exit_on_stop_opc and stop opc detected");
        terminate_simulator();
      } else {
        info(proc,"insn_num_tracing and insn_count reached");
        terminate_simulator();
      }
    }

    //First time reaching this insn count
    if(executed_instructions == insn_start) {
      info(proc,"trace start insn count reached PC:0x%lx\n",PC);

      _in_trace_region = true;

      auto  _xlen = proc->get_xlen();
      reg_t _satp = proc->get_state()->satp->read();
      reg_t _ppn = get_field(_satp, _xlen == 32 ? SATP32_PPN : SATP64_PPN);
      prog_ppn = (uint64_t) (_ppn & _PPN_MASK);

      if((bool)stf_writer == false)  {
        open_trace(proc,fetch);
      }

      record_machine_state(proc);
    }

    if(_in_trace_region) {

      if (executed_instructions + 1 == insn_start+warmup_size) {
         _traced_warmup_insns = _traced_instructions_running;
      }


      trace_element(proc,fetch,debug);
    }
  }
  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  void trace_macro_insn(processor_t *proc,insn_fetch_t &fetch,
                        std::string debug="")
  {
    _pending_region = false;
    auto const state = proc->get_state();
    auto const PC = state->pc;

    // The trace file is not closed here since we support non-contiguous 
    // regions. STOPs while already stopped are ignored
    if(is_stop_macro(fetch.insn.bits()) ) { //&& in_trace_region) {

      info(proc,"trace stop  opc detected 0x%lx\n",PC);

      //Optionally include the trace macros from the trace
      if (include_trace_macros && _in_trace_region) {
        trace_element(proc,fetch,debug);
      }

      report_stats(proc,debug);

      if(exit_on_stop_opc) {
        info(proc,"--stf_exit_on_stop_opc and stop opc detected");
        terminate_simulator();
      }
      
      _in_trace_region = false;
      _traced_instructions_region = 0;

      return;
    }

    // This is the start macro, this could be the 1st overall or the
    // start of a new region. We can tell it is the 1st if stf_writer
    // has not been initialized. For start of any region we record the
    // state of the machine on entry to the region.
    if(is_start_macro(fetch.insn.bits())) {

      info(proc,"trace start opc detected 0x%lx\n",PC);
      report_stats(proc,debug);

      auto  _xlen = proc->get_xlen();
      reg_t _satp = proc->get_state()->satp->read();
      reg_t _ppn = get_field(_satp, _xlen == 32 ? SATP32_PPN : SATP64_PPN);

      prog_ppn = (uint64_t) (_ppn & _PPN_MASK);

      //We are already in trace_macro_insn and so slow loop
      _pending_region = false;

      //start macro is the beginning of the trace region
      _in_trace_region = true;

      if((bool)stf_writer == false)  {
        open_trace(proc,fetch);
      }

      record_machine_state(proc);
  
      //Optionally exclude the trace macros from the trace
      if (!include_trace_macros) {
         stf_writer << stf::ForcePCRecord(last_npc);
	 return;
      }
    }

    if (_in_trace_region) trace_element(proc,fetch,debug);
  }

  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  void trace_element(processor_t *proc,insn_fetch_t &fetch,
                     std::string debug="") 
  {
    auto const state = proc->get_state();
    auto const PC = state->pc;
    //The open_trace is deferred until now to align with behavior
    //of Dromajo which produces known good trace files.
    if((bool)stf_writer == false)  {
      open_trace(proc,fetch);
    }

    //Trace this instruction if it has the right PRIV level and PPN
    bool priv_in_range = is_priv_mode_traceable(state, priv_modes);
    bool pending_exception = false; //TODO find this in spike

    auto  _xlen = proc->get_xlen();
    reg_t _satp = state->satp->read();
    reg_t _ppn = get_field(_satp,_xlen == 32 ? SATP32_PPN : SATP64_PPN);
    bool ppn_match = (reg_t) prog_ppn == _ppn;

    //Instruction number tracing ignores all predicates
    bool trace_this = (priv_in_range && !pending_exception && ppn_match);

    if(trace_this) {
        uint32_t insn_bytes = (fetch.insn.bits() & 0x3) == 0x3 ? 4 : 2;

        bool skip_record = false;

        if(is_taken_branch) {
          stf_writer << stf::InstPCTargetRecord(last_npc);
        }
       // In dromajo there was a possibility that the current instruction
        // will cause a page fault/timer interrupt/process switch so
        // the next instruction might not be on the programs path
        // TODO: determine if this is the case in Spike

        //TODO: remove the over-ride
        //skip_record = PC != proc->get_last_pc() + insn_bytes;

        if(!skip_record) {
          if(_trace_register_state) {
            emit_register_records(proc);
          }

          if(_trace_memory_records) {
            emit_memory_records(proc);
          }

          if(insn_bytes == 4) {
            stf_writer << stf::InstOpcode32Record(fetch.insn.bits());
          } else {
            stf_writer << stf::InstOpcode16Record(fetch.insn.bits()&_CMP_MASK);
          }
          ++_traced_instructions_region;
          ++_traced_instructions_running;
       }
    } else {
       stf_writer << stf::ForcePCRecord(state->pc);
    }
  }

  // ---------------------------------------------------------------- 
  // ---------------------------------------------------------------- 
  void info(processor_t *p,const char* fmt, ...)
  {
    if (!p->in_quiet_mode()) {
      fprintf(stderr, "-I: ");
      va_list args;
      va_start(args, fmt);
      vfprintf(stderr, fmt, args);
      va_end(args);
      fprintf(stderr, "\n");
    }
  }
  // ---------------------------------------------------------------- 
  // This is simple enough now to not bother creating a dependency on
  // a json emitter
  // ---------------------------------------------------------------- 
  bool write_json_stats(uint64_t traced_instructions_running,
                        uint64_t exec_instructions,
                        uint64_t instret_count,
                        double   duration_ms,
                        double   mips,
                        std::vector<long int> num_bbv_insns,
                        std::vector<uint64_t> roi_start_insn_cnt,
                        uint64_t warmup_region_insns)
  {
    std::ofstream jout(stats_file_name.c_str());

    if(!jout.is_open()) {
      std::cerr<<"Could not open "<<stats_file_name<<" for write"<<std::endl;
      return false;
    }

    jout<<"{"<<std::endl;
    jout<<"  \"stats\" : {"<<std::endl;

    jout<<"    \"traced_instructions\" : "
        << std::dec<< traced_instructions_running<<","<<std::endl;

    jout<<"    \"executed_instructions\" : "
        << std::dec<< exec_instructions<<","<<std::endl;

    jout<<"    \"instructions_retired\" : "
        << std::dec<< instret_count <<","<<std::endl;

    jout<<"    \"duration_ms\" : "
        << std::fixed << std::setprecision(3)<<duration_ms <<","<<std::endl;

    jout<<"    \"wall_clock_mips\" : "
        << std::fixed << std::setprecision(3) << mips <<","<<std::endl;

    for (size_t i=0; i<num_bbv_insns.size(); i++) {
        jout<<"    \"cpu" << std::dec << i << "_bbv_total_isns\" : "
            << num_bbv_insns[i] <<","<<std::endl;
    }

    for (size_t i=0; i<roi_start_insn_cnt.size(); i++) {
        jout<<"    \"cpu" << std::dec << i << "_bbv_roi_started\" : "
            << roi_start_insn_cnt[i] <<","<<std::endl;
    }

    jout<<"    \"warmup_region_instructions\" : "
        << std::dec<< warmup_region_insns <<std::endl;

    jout<<"  }"<<std::endl;
    jout<<"}"<<std::endl;

    jout.close();

    return true;
  }
  // ---------------------------------------------------------------- 
  // Note: debug function, so a portion ignores quiet_mode
  // ---------------------------------------------------------------- 
  void report_stats(processor_t *p,std::string debug="")
  {
    if(stf_verbose) {
      fprintf(stderr,"    %s\n",debug.c_str());
      fprintf(stderr,"-I: traced_instructions(region)  %" PRIu64 "\n",
                          _traced_instructions_region);
      fprintf(stderr,"-I: traced_instructions(running) %" PRIu64 "\n",
                          _traced_instructions_running);
      fprintf(stderr,"-I: executed_instructions %" PRIu64 "\n",
                          executed_instructions);
      fprintf(stderr,"-I: traced %ld instructions\n",
                          _traced_instructions_running);
    }

    info(p,"traced %ld instructions\n",_traced_instructions_running);
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  inline uint64_t bit_mask(size_t size) {
    size_t num_bits = (size <= 8 ? size : 0) * 8;
    return num_bits ? (uint64_t(1) << num_bits) - 1 : 0;
  }

  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  void emit_memory_records(processor_t *p) {
    auto const state = p->get_state();

    // Memory reads
    for(const auto &load : state->log_mem_read) {
      auto [addr, value, size] = load;

      stf_writer << stf::InstMemAccessRecord(addr,size,0,
                    stf::INST_MEM_ACCESS::READ);
      stf_writer << stf::InstMemContentRecord(value & bit_mask(size));
    }

    //TODO: this is a coordination problem with Spike's native logging
    //      scheme. Add check to make sure --stf_trace_memory_records
    //      is not enabled at the same time as --log-commits until a
    //      coordination test can be written.
    state->log_mem_read.clear();

    // Memory writes
    for(const auto &store : state->log_mem_write) {
      auto [addr, value, size] = store;

      stf_writer << stf::InstMemAccessRecord(addr,size, 0,
                    stf::INST_MEM_ACCESS::WRITE);
      stf_writer << stf::InstMemContentRecord((long int) value);
    }

    //TODO see above
    state->log_mem_write.clear();
  }

  // ----------------------------------------------------------------
  // Trace updates to the register state caused by insn execution
  // CSRs are not traced; side effects, if any, are not traced
  // These are expensive in terms of model performance
  // ----------------------------------------------------------------
  void emit_register_records(processor_t *p) {
    auto const state = p->get_state();
    //r is std::map<reg_t, freg_t> commit_log_reg_t
    for(auto r : state->log_reg_write) {
      auto dest_raw = r.first;
      auto dest_type = (dest_raw & 1) ? stf::Registers::STF_REG_TYPE::FLOATING_POINT :
                                        stf::Registers::STF_REG_TYPE::INTEGER;
      stf_writer << stf::InstRegRecord(dest_raw >> 4,
            dest_type,
            stf::Registers::STF_REG_OPERAND_TYPE::REG_DEST,
            r.second.v[0]);
    }
    state->log_reg_write.clear();
  }

  // ----------------------------------------------------------------
  // Trace the register state, typically on change in control flow
  // ----------------------------------------------------------------
  void record_machine_state(processor_t *p) {

    auto const state = p->get_state();
    stf_writer << stf::ForcePCRecord(state->pc);

    //TODO once dromajo comparison is done this if statement should
    //be removed, when 
    if(_trace_register_state) {

      //NXPR declared in decode.h
      for(size_t x=0;x<NXPR;++x) {
        stf_writer << stf::InstRegRecord(x,  
          stf::Registers::STF_REG_TYPE::INTEGER,
          stf::Registers::STF_REG_OPERAND_TYPE::REG_STATE,
          state->XPR[x]);
      }

      //NFPR declared in decode.h
      //uint64_t hi = *reinterpret_cast<const uint64_t*>(&state->FPR[f]+1);
      if(p->get_flen() > 0 && p->get_flen() <= 64) {

        for(size_t f=0;f<NFPR;++f) {
          uint64_t lo = *reinterpret_cast<const uint64_t*>(&state->FPR[f]);
          stf_writer << stf::InstRegRecord(f,
            stf::Registers::STF_REG_TYPE::FLOATING_POINT,
            stf::Registers::STF_REG_OPERAND_TYPE::REG_STATE,
            lo);
        }

      } else if(p->get_flen() > 64) {
        //TODO: f128_t is also a problem for stf_lib
        fprintf(stderr,
           "-E: found FLEN = %d, FLEN > 64 is not supported in this version\n",
            p->get_flen());
        assert(0);
      }

      //TODO add support for dumping vector registers
      //if(p->VU.get_vlen() > 0) {
      //}
    }
  }

  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  void open_trace(processor_t *proc,insn_fetch_t &fetch) {

    stf_writer.open(trace_file_name);

    std::string spike_sha = "SPIKE SHA:0";
    std::string stf_lib_sha = "STF_LIB SHA:0";

    uint32_t vMajor = 0;
    uint32_t vMinor = 0;
    uint32_t vPatch = 0;

    //The version and SHA's are written to the trace
    if(!force_zero_sha) {
      spike_sha   = std::string("SPIKE SHA:")+std::string(STF_SPIKE_GIT_SHA);
      stf_lib_sha = std::string("STF_LIB SHA:")+std::string(STF_LIB_GIT_SHA);
      vMajor = STF_SPIKE_VERSION_MAJOR;
      vMinor = STF_SPIKE_VERSION_MINOR;
      vPatch = STF_SPIKE_VERSION_PATCH;
    }

    stf_writer.addTraceInfo(stf::TraceInfoRecord(
      stf::STF_GEN::STF_GEN_SPIKE,vMajor,vMinor,vPatch,spike_sha)
    );

    stf_writer.addHeaderComment(stf_lib_sha);

    stf_writer.setISA(stf::ISA::RISCV);

    auto  _xlen = proc->get_xlen();
    if(_xlen == 64) {
      stf_writer.setHeaderIEM(stf::INST_IEM::STF_INST_IEM_RV64);
      stf_writer.setTraceFeature(stf::TRACE_FEATURES::STF_CONTAIN_RV64);
    } else if(_xlen == 32) {
      stf_writer.setHeaderIEM(stf::INST_IEM::STF_INST_IEM_RV32);
    } else if(_xlen == 128) {
      fprintf(stderr, //TODO: ? add 128 to stf_lib ?
        "-E: stf tracing is limited to RV32 and RV64, RV128 found\n");
      assert(0);
    } 

    //TODO add support for Vector - add the STF_VLEN_CONFIG record

    stf_writer.setTraceFeature(
      stf::TRACE_FEATURES::STF_CONTAIN_PHYSICAL_ADDRESS
    );

    stf_writer.setHeaderPC(proc->get_state()->pc);
    stf_writer.finalizeHeader();
  }

  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  void close_trace() {
    stf_writer.flush();
    stf_writer.close();
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool in_traceable_region() { return _in_trace_region || _pending_region; }
  // ----------------------------------------------------------------
  bool is_start_of_region(uint32_t bits )  { 
    if(   (insn_num_tracing && executed_instructions >= insn_start)
       || (macro_tracing    && is_start_macro(bits)))
    {
      _pending_region = true;
      return true;
    }
    return false;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool is_start_macro(uint32_t bits) {
    _pending_region = true;
    return bits == _START_TRACE;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool is_stop_macro (uint32_t bits) {
    _pending_region = false;
    return bits == _STOP_TRACE;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool is_priv_mode_traceable(const state_t* state, const std::string& priv_string) {

    char priv_mode = '\0';

    uint32_t vpriv = state->last_inst_priv;

    switch (vpriv) {
      case 0: priv_mode = 'U'; break;
      case 1: priv_mode = 'S'; break;
      case 3: priv_mode = 'M'; break;
      default:
         fprintf(stderr,"Could not determine privilege mode\n");
         break;
    }

    // hypervisor mode?
    if ((priv_mode == 'S') && (!state->prev_v)) {
      priv_mode = 'H';
    }

    for (char mode : priv_string) {
      if (priv_mode == mode) { return true; }
    }

    return false;
  }
  // ----------------------------------------------------------------
  // option support methods
  // ----------------------------------------------------------------
  #define E(s) fprintf(stderr,s);
  void stf_help()
  {
    std::string header="";
    header.append(75,'-');
    //stf_trace options
    fprintf(stderr,"  %s\n",header.c_str());
    E("  STF Trace options\n");
    E("    - STF options are ignored if --stf_trace is not specified.\n");
    E("    - STF tracing supports a single cpu \n");
    fprintf(stderr,"  %s\n",header.c_str());
    E("  --stf_trace <file>     Dump an STF trace to the given file.\n");
    E("                         Use .zstf extension for compressed trace\n");
    E("                         output. Use .stf for uncompressed output\n");
    E("  --stf_macro_tracing    Enable STF tracing on START/STOP macros.\n");
    E("                         stf_macro_tracing and stf_insn_num_tracing\n");
    E("                         are exclusive.\n");
    E("                         (default false)\n");
    E("  --stf_insn_num_tracing Enable STF tracing on instruction count.\n");
    E("                         stf_macro_tracing and stf_insn_num_tracing\n");
    E("                         are exclusive.\n");
    E("                         (default false)\n");
    E("  --stf_insn_start <N>   Start STF tracing after N instructions.\n");
    E("  --stf_insn_count <N>   Terminate STF tracing after N instructions\n");
    E("                         from stf_insn_start.\n");
    E("  --stf_exit_on_stop_opc Terminate the simulation after detecting a\n");
    E("                         STOP_TRACE opcode. Using this switch\n");
    E("                         disables non-contiguous region tracing.\n");
    E("                         (default false)\n");
    E("  --stf_trace_register_state\n");
    E("                         Include changes to register state through \n");
    E("                         instruction execution in the STF output.\n");
    E("                         Implies --log_commits.\n");
    E("                         (default false)\n");
    E("                         Note: changes in control flow emit full\n");
    E("                         register state regardless of this setting.\n");
    E("  --stf_trace_memory_records\n");
    E("                         Include memory records in the STF trace.\n");
    E("                         (default false)\n");
    E("  --stf_priv_modes <M|H|S|U>\n");
    E("                         Specify which privilege modes to include\n");
    E("                         in the trace. Accepts any combination of\n");
    E("                         M,H,S, and U (default USHM)\n");
    E("  --stf_warmup_size <N>  Record the number of --stf_priv_modes instructions\n");
    E("                         in the first --stf_warmup_size total instructions.\n");
    E("  --stf_force_zero_sha   Emit 0 for all SHA's in the STF header.\n");
    E("                         For regression and other testing purposes\n");
    E("                         (default false)\n");
    E("  --stf_include_macros   Include the trace macros in the trace.\n");
    E("                         These are:  START:  xor x0,x0,x0\n");
    E("                                     STOP:   xor x0,x1,x1\n");
    E("                         (default false)\n");
    E("  --stf_stats <file>     Name of file containing execution stats. Format is json\n");
    E("                         (default exe_stats.json)\n");

    //Hidden option
    //E("  --stf_verbose          Verbose console message.\n");
    //E("                         (default false)\n");
  }
  #undef E
  // -------------------------------------------------------------------------
  void set_options(option_parser_t &parser)
  {
    parser.option(0,"stf_trace", 1, [&](const char* s) {
      trace_file_name = s;
    });

    parser.option(0,"stf_exit_on_stop_opc", 0, [&](const char UNUSED *s){
      exit_on_stop_opc = true;
    });

    parser.option(0,"stf_trace_register_state", 0, [&](const char UNUSED *s){
      _trace_register_state = true;
    });

    parser.option(0,"stf_trace_memory_records", 0, [&](const char UNUSED *s){
      _trace_memory_records = true;
    });

    parser.option(0,"stf_priv_modes", 1, [&](const char* s){
      priv_modes = s;
    });

    parser.option(0,"stf_force_zero_sha", 0, [&](const char UNUSED *s){
      force_zero_sha = true;
    });

    parser.option(0,"stf_macro_tracing", 0, [&](const char UNUSED *s){
      macro_tracing = true;
    });

    parser.option(0,"stf_insn_num_tracing", 0, [&](const char UNUSED *s){
      insn_num_tracing = true;
    });

    parser.option(0,"stf_insn_start", 1, [&](const char* s){
      insn_start = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_insn_count", 1, [&](const char* s){
      insn_count = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_warmup_size", 1, [&](const char* s){
      warmup_size = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_include_macros", 0, [&](const char UNUSED *s){
      include_trace_macros = true;
    });

    parser.option(0,"stf_verbose", 0, [&](const char UNUSED *s){
      stf_verbose = true;
    });

    parser.option(0,"stf_stats", 1, [&](const char* s) {
      stats_file_name = s;
    });

  }
  // -------------------------------------------------------------------------
  // At present checks are simple
  // -------------------------------------------------------------------------
  bool option_checks(cfg_t &cfg) {

    bool ok = true;
    if(macro_tracing && insn_num_tracing) {
      fprintf(stderr,"\n-E: --stf_macro_tracing && --stf_insn_num_tracing are "
                     "mutually exclusive\n\n");
      ok = false;
    }

    if(!trace_file_name.empty() && cfg.nprocs() > 1 ) {
      fprintf(stderr,"\n-E: STF tracing supports only a single cpu, "
                     "%d specified.\n\n", (int)cfg.nprocs());
      ok = false;
    }

    return ok;
  }

  // more singleton 
  static StfHandler *instance;

public:
  std::string trace_file_name{""};
  std::string stats_file_name{"exe_stats.json"};

  bool exit_on_stop_opc{false};
  bool stf_verbose{false};

  bool first_force{true};


  bool force_zero_sha{false};
  bool include_trace_macros{false};

  bool macro_tracing{false};        //trace mode flag
  bool insn_num_tracing{false};     //trace mode flag

  uint64_t insn_start{0};           //limit
  uint64_t insn_count{UINT64_MAX};  //limit
  uint64_t warmup_size{0};

  std::string priv_modes{"USHM"};
  stf::STFWriter stf_writer;

  //Run time options
  uint32_t highest_priv_mode{0};
  int64_t  prog_ppn{-1};

  //This flag is used to exit fast loop and enter slow loop.
  //This is set when start macro has been detected

  bool trace_file_open{false};
  uint64_t executed_instructions{0};
  uint64_t last_npc{0};
  uint32_t insn_bytes{0};
  bool     is_taken_branch{false};

private:
  bool _pending_region{false};
  bool _in_trace_region{false};
  bool _trace_memory_records{false};
  bool _trace_register_state{false};
  uint64_t _traced_instructions_region{0};
  uint64_t _traced_instructions_running{0};
  uint64_t _traced_warmup_insns{0};

  static constexpr uint32_t _START_TRACE = 0x00004033; //xor x0,x0,x0
  static constexpr uint32_t _STOP_TRACE  = 0x0010c033; //xor x0,x1,x1

  static constexpr uint64_t _PPN_MASK   = 0x00000FFFFFFFFFFF;
  static constexpr uint32_t _CMP_MASK    = 0x0000FFFF;
  // ----------------------------------------------------------------
  // more singleton 
  // ----------------------------------------------------------------
  StfHandler() {} //default
  StfHandler(const StfHandler&) = delete; //copy
  StfHandler(StfHandler&&)      = delete; //move
  StfHandler& operator=(const StfHandler&) = delete; //assignment

};

extern std::shared_ptr<StfHandler> stfhandler;
