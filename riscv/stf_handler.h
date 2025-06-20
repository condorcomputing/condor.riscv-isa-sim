// --------------------------------------------------------------------------
// Copyright (C) 2024, Condor Computing
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
#include "stf_tracer.h"

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
  //Do any required cleanup before forcing exit, nothing currently
  // ----------------------------------------------------------------
  void terminate_simulator() {
    throw stf_trace_complete("STF trace capture complete.\n");
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool stf_writer_enabled() {
    for (auto tracer : tracers) {
      if (tracer->stf_writer_enabled()) {
        return true;
      }
    }
    return false;
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  void report_stats(sim_t &s,cfg_t &cfg,
       time_point<high_resolution_clock> &start)
  {
    for (auto tracer : tracers) {
      tracer->report_stats(s, cfg, start);
    }
  }
  // ----------------------------------------------------------------
  // Getters
  // ----------------------------------------------------------------
  bool trace_memory_records()    {
    for (auto tracer : tracers) {
      if (tracer->trace_memory_records()) {
        return true;
      }
    }
    return false;
  }
  bool trace_register_state()    {
    for (auto tracer : tracers) {
      if (tracer->trace_register_state()) {
        return true;
      }
    }
    return false;
  }
  bool stf_enable_log_commits()  {
    for (auto tracer : tracers) {
      if (tracer->stf_enable_log_commits()) {
        return true;
      }
    }
    return false;
  }
  // ----------------------------------------------------------------
  // Common trace instruction method which selects specific trace
  // method based on trace mode
  // ----------------------------------------------------------------
  void trace_insn(processor_t *p,insn_fetch_t &fetch,
                  reg_t pc, reg_t npc, std::string debug="")
  {
    for (auto tracer : tracers) {
      if (tracer->in_traceable_region()) {
        try {
          tracer->trace_insn(p, fetch, pc, npc, debug);
        }
        catch(stf_trace_complete &e) {
          std::cout << e.what();

          if(tracer->stf_writer_enabled()) {
            tracer->close_trace();
          }

          if (tracer->trace_file_name != "") {
            // FIXME
            // tracer->report_stats(p->sim,p->cfg,high_resolution_clock::now());
          }

          bool all_trace_complete = true;
          for (auto tracer : tracers) {
            if (!tracer->_trace_complete) {
              all_trace_complete = false;
              break;
            }
          }
          if (all_trace_complete) {
            throw;
          }
        }
      }
    }
  }

  // ----------------------------------------------------------------
  // Common event tracing method, can be called on any event,
  // in any trace mode, whether in trace region or not.
  // ----------------------------------------------------------------
  void trace_event(processor_t *p,insn_fetch_t &fetch,
                  reg_t pc, reg_t npc, trap_t t, std::string debug="")
  {
    for (auto tracer : tracers) {
      if (tracer->in_traceable_region()) {
        tracer->trace_event(p, fetch, pc, npc, t, debug);
      }
    }
  }
  // ----------------------------------------------------------------
  // Note: debug function, so a portion ignores quiet_mode
  // ----------------------------------------------------------------
  void report_stats(processor_t *p,std::string debug="")
  {
    for (auto tracer : tracers) {
      tracer->report_stats(p, debug);
    }
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  void close_trace() {
    for (auto tracer : tracers) {
      tracer->close_trace();
    }
  }
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  bool in_traceable_region() {
    for (auto tracer : tracers) {
      if (tracer->in_traceable_region()) {
        return true;
      }
    }
    return false;
  }
  // ----------------------------------------------------------------
  bool is_start_of_region(uint32_t bits )  {
    bool _is_start = false;
    for (auto tracer : tracers) {
      if (tracer->is_start_of_region(bits)) {
         _is_start = true;
      }
    }
    return _is_start;
  }
  void incr_executed_instructions() {
    for (auto tracer : tracers) {
      ++tracer->executed_instructions;
    }
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
    E("  --stf_count_from_bbv_roi\n");
    E("                         --stf_insn_start/--stf_insn_count values are\n");
    E("                         relative to the BBV simpoint CSR enable write\n");
    E("                         instruction.\n");
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
      if (!stf_trace_opts_ooo && tracer_cfg_started) {
         create_tracer();
         reset_default_opts();
      } else if (stf_trace_opts_ooo && trace_file_name != "") {
         std::cerr << "-E --stf_trace must be specified before trace options" << std::endl;
         exit(1);
      }

      trace_file_name = s;
      tracer_cfg_started = true;
    });

    parser.option(0,"stf_exit_on_stop_opc", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      exit_on_stop_opc = true;
    });

    parser.option(0,"stf_trace_register_state", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      _trace_register_state = true;
    });

    parser.option(0,"stf_trace_memory_records", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      _trace_memory_records = true;
    });

    parser.option(0,"stf_priv_modes", 1, [&](const char* s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      priv_modes = s;
    });

    parser.option(0,"stf_force_zero_sha", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      force_zero_sha = true;
    });

    parser.option(0,"stf_macro_tracing", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      macro_tracing = true;
    });

    parser.option(0,"stf_insn_num_tracing", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      insn_num_tracing = true;
    });

    parser.option(0,"stf_insn_start", 1, [&](const char* s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      insn_start = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_insn_count", 1, [&](const char* s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      insn_count = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_count_from_bbv_roi", 0, [&](const char* s){
      if (!tracer_cfg_started) {
         std::cerr << "-E --stf_trace must be specified before trace options" << std::endl;
         exit(1);
      }
      count_from_bbv_roi = true;
    });

    parser.option(0,"stf_warmup_size", 1, [&](const char* s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      warmup_size = strtoull(s, nullptr, 0);
    });

    parser.option(0,"stf_include_macros", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      include_trace_macros = true;
    });

    parser.option(0,"stf_verbose", 0, [&](const char UNUSED *s){
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      stf_verbose = true;
    });

    parser.option(0,"stf_stats", 1, [&](const char* s) {
      if (!tracer_cfg_started) {
        stf_trace_opts_ooo = true;
      }
      stats_file_name = s;
    });

  }
  // -------------------------------------------------------------------------
  // At present checks are simple
  // -------------------------------------------------------------------------
  bool option_checks(cfg_t &cfg, bool bbv_en) {

    // if bbv is enabled, create at least one tracer instance for stat generation.
    if (tracer_cfg_started || stf_trace_opts_ooo || (bbv_en && tracers.size()==0)) {
      create_tracer();
      tracer_cfg_started = false;
    }

    bool ok = true;
    for (auto tracer : tracers) {
      ok &= tracer->option_checks(cfg);
    }
    return ok;
  }

  void simpoint_csr_write_notify(processor_t *const proc, const reg_t value) {
    for (auto tracer : tracers) {
      tracer->simpoint_csr_write_notify(proc, value);
    }
  }

  // more singleton 
  static StfHandler *instance;

  void reset_default_opts() {
    trace_file_name = "";
    stats_file_name = "exe_stats.json";

    exit_on_stop_opc = false;
    stf_verbose = false;

    force_zero_sha = false;
    include_trace_macros = false;

    macro_tracing = false;
    insn_num_tracing = false;

    insn_start = 0;
    insn_count = UINT64_MAX;
    warmup_size = 0;
    count_from_bbv_roi = false;

    priv_modes = "USHM";
  }

  void create_tracer() {
          std::cerr << "Creating tracer for " << trace_file_name << std::endl;
    StfTracer* tracer = new StfTracer
      (
        trace_file_name,
        exit_on_stop_opc,
        _trace_register_state,
        _trace_memory_records,
        priv_modes,
        force_zero_sha,
        macro_tracing,
        insn_num_tracing,
        insn_start,
        insn_count,
        warmup_size,
        count_from_bbv_roi,
        include_trace_macros,
        stf_verbose,
        stats_file_name
      );
    tracers.push_back(tracer);
  }

public:
  std::string trace_file_name{""};
  std::string stats_file_name{"exe_stats.json"};

  bool exit_on_stop_opc{false};
  bool stf_verbose{false};

  bool force_zero_sha{false};
  bool include_trace_macros{false};

  bool macro_tracing{false};        //trace mode flag
  bool insn_num_tracing{false};     //trace mode flag

  uint64_t insn_start{0};           //limit
  uint64_t insn_count{UINT64_MAX};  //limit
  uint64_t warmup_size{0};
  bool count_from_bbv_roi{false};

  std::string priv_modes{"USHM"};

  bool _trace_memory_records{false};
  bool _trace_register_state{false};

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
  // ----------------------------------------------------------------
  // more singleton 
  // ----------------------------------------------------------------
  StfHandler() {} //default
  StfHandler(const StfHandler&) = delete; //copy
  StfHandler(StfHandler&&)      = delete; //move
  StfHandler& operator=(const StfHandler&) = delete; //assignment
  std::vector<StfTracer*> tracers;
  bool tracer_cfg_started{false};
  bool stf_trace_opts_ooo{false};

};

extern std::shared_ptr<StfHandler> stfhandler;
