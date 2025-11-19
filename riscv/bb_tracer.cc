#include "bb_tracer.h"
#include "processor.h"
#include "simif.h"
#include <iostream>

namespace bb_ctrl {
    simpoint_csr_t::simpoint_csr_t(processor_t *const proc, const reg_t addr, const reg_t init) :
            csr_t(proc, addr),
            val(init) {
    }

    bool simpoint_csr_t::unlogged_write(const reg_t value) noexcept {
        std::cerr << "Write to BB Tracer CSR observed" << std::endl;

        // For STF trace:
        proc->simpoint_csr_write_notify(value);

        proc->get_bb_tracer().handle_simpoint_macro(proc->get_last_pc(), value, proc->get_state()->minstret->read());
        return true;
    }
} // namespace bb_ctrl

bb_tracer::bb_tracer(processor_t* proc, bool en_bbv, const std::string &bb_file_base_name,
                     uint64_t simpoint_size, uint64_t warmup_size, uint32_t heart_id) : m_proc(proc),
                                                                  m_simpoint_size(simpoint_size),
                                                                  m_next_id(1),
                                                                  m_next_bbv_dump(simpoint_size+1),
                                                                  m_heart_id(heart_id),
                                                                  m_en_bbv(en_bbv),
                                                                  m_warmup_size(warmup_size) {

    if (m_en_bbv) {
        auto bb_file_name = bb_file_base_name + "_cpu" + std::to_string(m_heart_id);
        m_bb_file.open(bb_file_name, std::ios::out);
        if (m_bb_file.is_open()) {
            std::cout << "Opened file: " << bb_file_name << " to write BBV for CPU: " << m_heart_id << "\n";
        } else {
            std::cout << "Failed to open " << bb_file_name << " file! Disabling BB capture for CPU: " << m_heart_id
                      << "\n";
            m_en_bbv = false;
        }
    }

    if (m_en_bbv || bb_file_base_name != "") {
        auto bb_tracks_file_name = bb_file_base_name + '_' + std::to_string(m_heart_id) + "_tracks.log" ;
        m_bb_tracks_file.open(bb_tracks_file_name, std::ios::out);
        if (m_bb_tracks_file.is_open()) {
            std::cout << "Opened file: " << bb_tracks_file_name << " to log BBV snippet tracks for CPU: " << m_heart_id << "\n";
        } else {
            std::cout << "Failed to open " << bb_tracks_file_name << " file! Disabling BB capture for CPU: " << m_heart_id
                      << "\n";
            m_en_bbv = false;
        }
    }
}


bb_tracer::~bb_tracer() {
    m_bb_file.close();
    m_bb_tracks_file.close();
}

void bb_tracer::flush_bb_vector(const uint64_t steps) {
    instr_cnt += steps;
    if (instr_cnt >= m_next_bbv_dump) {
        m_next_bbv_dump += m_simpoint_size;

        if (bb_tracer_options::start_interval) {
            uint64_t interval = ((m_total_insn_in_roi) / m_simpoint_size);
            if (bb_tracer_options::start_interval >= interval) {
                m_bbv.clear();
            }
        }

        if (!m_bbv.empty()) {
            std::ostringstream oss;
            oss << "T";
            for (const auto& ent: m_bbv) {
                if (unlikely(bb_tracer_options::encode_bb_ids)) {
                    auto it = m_pc2id.find(ent.first);
                    uint64_t id = 0u;
                    if (it == m_pc2id.end()) {
                        id = m_next_id++;
                        m_pc2id[ent.first] = id;
                    } else {
                        id = it->second;
                    }
                    oss << ":" << id << ":" << ent.second << " ";
                } else {
                    oss << ":" << ent.first << ":" << ent.second << " ";
                }
            }
            oss << "\n";

            m_bb_file << oss.str();
            m_bb_file.flush();
            bbv_lines.push_back(oss.str());
            m_bbv.clear();
        }

        if (bb_tracer_options::max_intervals && (m_total_insn_in_roi >= (bb_tracer_options::start_interval + bb_tracer_options::max_intervals) * m_simpoint_size)) {
            throw bb_ctrl::simpoint_terminate("Collected specificed maximum of " + std::to_string(bb_tracer_options::max_intervals) + " intervals.  " +
                                              "Benchmark exited with code: " +
                                              std::to_string(m_benchmark_return_code) + "\n");
        }
    }
}

int bb_tracer::capture_basic_block(const uint64_t pc) {

    if ((m_last_pc + 2) != pc && (m_last_pc + 4) != pc) {
        m_bbv[m_last_pc] += m_ninst;
        m_ninst = 0;
    }

    return 1; // Compatibility with original
}

void bb_tracer::simpoint_step(uint64_t steps, uint64_t pc) {
    if (bb_tracer_options::bbv_umode_only) {
        // TODO ppn can be saved & read only after CSR instructions
        auto  _xlen = m_proc->get_xlen();
        reg_t _satp = m_proc->get_state()->satp->read();
        reg_t _ppn = get_field(_satp,_xlen == 32 ? SATP32_PPN : SATP64_PPN);

        if (_ppn != m_ppn) {
            return;
        }
    }
    if (m_last_pc && m_simpoint_roi && m_last_pc != m_simpoint_en_pc) {
        capture_basic_block(pc);
        flush_bb_vector(steps);
        m_total_insn_in_roi++;
        m_ninst++;
        log_simpoint_tracks(pc);
    } else if (m_simpoint_roi && pc != m_simpoint_en_pc) {
        m_total_insn_in_roi++;
        m_ninst++;
        flush_bb_vector(steps);
    }
    if (m_simpoint_roi && pc != m_simpoint_en_pc) {
        if (m_last_pc == m_simpoint_en_pc) {
            log_simpoint_tracks(pc);
        }
        m_last_pc = pc;
    }
    if(unlikely(m_terminate)){
        throw bb_ctrl::simpoint_terminate("Reached termination instruction in benchmark. Benchmark exited with code: " +
                                          std::to_string(m_benchmark_return_code) + "\n");
    }
}

bool bb_tracer::in_region_of_interest() const { return (m_en_bbv && m_simpoint_roi); }

long int bb_tracer::get_total_insns() const {
    return m_total_insn_in_roi;
}

uint64_t bb_tracer::get_insn_count_on_roi_start() const {
  return m_insn_num_roi_started;
}

uint64_t bb_tracer::get_benchmark_ppn() const {
  return m_ppn;
}

void bb_tracer::handle_simpoint_macro(uint64_t pc, const reg_t val, const uint64_t executed_insn_cnt) noexcept {
    if(m_en_bbv)
    {
        if ((val & 3) == 2) {
            std::cerr << "simpoint terminate\n";
            m_benchmark_return_code = val >> 2;
            m_terminate = true;
        } else if ((val & 3) == 1 && m_simpoint_roi) {
            std::cerr << "simpoint ROI already started\n";
        } else if ((val & 3) == 0 && m_simpoint_roi) {
            std::cerr << "simpoint ROI finished\n";
            capture_basic_block(0);
            flush_bb_vector(1u);
            m_simpoint_roi = false;
            m_last_pc = 0;
        } else if ((val & 3) == 0 && !m_simpoint_roi) {
            std::cerr << "simpoint ROI already finished\n";
        } else {
            std::cerr << "simpoint ROI started\n";
            m_simpoint_roi = true;
            m_simpoint_en_pc = pc;
            m_last_pc = pc;
            m_total_insn_in_roi = 0;
            m_insn_num_roi_started = executed_insn_cnt;

            auto  _xlen = m_proc->get_xlen();
            reg_t _satp = m_proc->get_state()->satp->read();
            m_ppn = get_field(_satp, _xlen == 32 ? SATP32_PPN : SATP64_PPN);
        }
        std::cerr.flush();
    } else {
        if ((val & 3) == 1) {
            // Log start instruction and ppn even when BB is not being traced.
            m_insn_num_roi_started = executed_insn_cnt;

            auto  _xlen = m_proc->get_xlen();
            reg_t _satp = m_proc->get_state()->satp->read();
            m_ppn = get_field(_satp, _xlen == 32 ? SATP32_PPN : SATP64_PPN);

            m_simpoint_roi = true;
        }
    }
}

void bb_tracer::log_simpoint_start_track(uint64_t pc) {
  snippet_start_insn_track.total_instr_count = m_proc->get_executed_insns();
  snippet_start_insn_track.total_umode_instr_count = m_proc->get_executed_umode_insns();
  snippet_start_insn_track.roi_instr_count = m_proc->get_executed_roi_umode_insns();
  //snippet_start_insn_track.roi_umode_instr_count =   // TODO
  snippet_start_insn_track.pc = pc;
}

void bb_tracer::log_simpoint_warmup_insn_track(uint64_t pc) {
  snippet_warmup_insn_track = next_snippet_warmup_insn_track;
  next_snippet_warmup_insn_track.total_instr_count = m_proc->get_executed_insns();
  next_snippet_warmup_insn_track.total_umode_instr_count = m_proc->get_executed_umode_insns();
  next_snippet_warmup_insn_track.roi_instr_count = m_proc->get_executed_roi_umode_insns();
  next_snippet_warmup_insn_track.pc = pc;
}

void bb_tracer::log_simpoint_end_insn_track(uint64_t pc) {
  // Last instruction in snippet
  snippet_end_insn_track.total_instr_count = m_proc->get_executed_insns();
  snippet_end_insn_track.total_umode_instr_count = m_proc->get_executed_umode_insns();
  snippet_end_insn_track.roi_instr_count = m_proc->get_executed_roi_umode_insns();
  //snippet_end_insn_track.roi_umode_instr_count =   // TODO
  snippet_end_insn_track.pc = pc;

  if (bb_tracer_options::start_interval) {
      uint64_t interval = ((m_total_insn_in_roi) / m_simpoint_size);
      if (bb_tracer_options::start_interval >= interval) {
          return;
      }
  }

  if (m_bb_tracks_file) {
    std::ostringstream oss;
    if (m_warmup_size) {
        oss << std::dec << snippet_warmup_insn_track.total_instr_count << " ";
        oss << std::dec << snippet_warmup_insn_track.total_umode_instr_count << " ";
        oss << std::dec << snippet_warmup_insn_track.roi_instr_count << " ";
        oss << std::hex << snippet_warmup_insn_track.pc << " ";
    }

    oss << std::dec << snippet_start_insn_track.total_instr_count << " ";
    oss << std::dec << snippet_start_insn_track.total_umode_instr_count << " ";
    oss << std::dec << snippet_start_insn_track.roi_instr_count << " ";
    oss << std::hex << snippet_start_insn_track.pc << " ";

    oss << std::dec << snippet_end_insn_track.total_instr_count << " ";
    oss << std::dec << snippet_end_insn_track.total_umode_instr_count << " ";
    oss << std::dec << snippet_end_insn_track.roi_instr_count << " ";
    oss << std::hex << snippet_end_insn_track.pc << std::endl;

    m_bb_tracks_file << oss.str();
    m_bb_tracks_file.flush();
    bbv_tracks.push_back(oss.str());
  }
}

void bb_tracer::log_simpoint_tracks(uint64_t pc) {
    // Log info on first/last instruction in each snippet
    if (m_total_insn_in_roi == m_next_bbv_dump-1) {
        log_simpoint_end_insn_track(pc);
    } else if (m_total_insn_in_roi == m_next_bbv_dump - m_simpoint_size) {
        // First instruction in snippet
        log_simpoint_start_track(pc);
    } else if (m_next_bbv_dump - m_total_insn_in_roi == m_warmup_size) {
        log_simpoint_warmup_insn_track(pc);
    }
}

json bb_tracer::checkpoint() {
  json j;

  j["m_ninst"] = m_ninst;
  j["m_next_bbv_dump"] = m_next_bbv_dump;
  j["m_simpoint_roi"] = m_simpoint_roi;
  j["m_last_pc"] = m_last_pc;
  j["m_simpoint_en_pc"] = m_simpoint_en_pc;
  j["m_total_insn_in_roi"] = m_total_insn_in_roi;
  j["m_insn_num_roi_started"] = m_insn_num_roi_started;
  j["ppn"] = m_ppn;

  j["m_bbv"] = m_bbv;
  j["m_pc2id"] = m_pc2id;
  j["m_next_id"] = m_next_id;
  j["m_heart_id"] = m_heart_id;
  j["snippet_warmup_insn_track"] = snippet_warmup_insn_track.checkpoint();
  j["next_snippet_warmup_insn_track"] = next_snippet_warmup_insn_track.checkpoint();
  j["snippet_start_insn_track"] = snippet_start_insn_track.checkpoint();
  j["snippet_end_insn_track"] = snippet_end_insn_track.checkpoint();
  j["instr_cnt"] = instr_cnt;

  if (bbv_lines.size()) {
    j["bbv_lines"] = bbv_lines;
  }

  if (bbv_tracks.size()) {
    j["bbv_tracks"] = bbv_tracks;
  }

  return j;
}

void bb_tracer::checkpoint_restore(json j) {
  m_ninst = j["m_ninst"];
  m_next_bbv_dump = j["m_next_bbv_dump"];
  m_simpoint_roi = j["m_simpoint_roi"];
  m_last_pc = j["m_last_pc"];
  m_simpoint_en_pc = j["m_simpoint_en_pc"];
  m_total_insn_in_roi = j["m_total_insn_in_roi"];
  flush_instr_cnt = j["m_total_insn_in_roi"];
  m_insn_num_roi_started = j["m_insn_num_roi_started"];
  m_ppn = j["ppn"];

  if (j.contains("m_bbv")) {
     m_bbv = j["m_bbv"];
     m_pc2id = j["m_pc2id"];
     m_next_id = j["m_next_id"];
     m_heart_id = j["m_heart_id"];

     snippet_warmup_insn_track.checkpoint_restore(j["snippet_warmup_insn_track"]);
     next_snippet_warmup_insn_track.checkpoint_restore(j["next_snippet_warmup_insn_track"]);
     snippet_start_insn_track.checkpoint_restore(j["snippet_start_insn_track"]);
     snippet_end_insn_track.checkpoint_restore(j["snippet_end_insn_track"]);
     instr_cnt = j["instr_cnt"];
  }

  if (j.contains("bbv_lines")) {
     bbv_lines = j["bbv_lines"];
  }

  if (j.contains("bbv_tracks")) {
     bbv_tracks = j["bbv_tracks"];
  }

  for (auto s : bbv_lines) {
    m_bb_file << s;
  }

  for (auto s : bbv_tracks) {
    m_bb_tracks_file << s;
  }

  checkpoint_restored = true;
}

namespace bb_tracer_options {
    bool en_bbv = false;
    std::string bb_file{""};
    uint64_t simpoint_size = 100000000UL;
    bool bbv_umode_only = false;
    uint64_t warmup_size = 0;
    uint64_t max_intervals = 0;
    uint64_t start_interval = 0;
    bool encode_bb_ids = false;
    bool checkpoint_restore = false;

    void set_options(option_parser_t &parser) {
        parser.option(0, "en_bbv", 0, [&](const char UNUSED *s) { en_bbv = true; });
        parser.option(0, "bb_file", 1,
                      [&](const char *s) { bb_tracer_options::bb_file = std::string(s); });
        parser.option(0, "simpoint_size", 1, [&](const char *s) { simpoint_size = strtoul(s, nullptr, 10); });
        parser.option(0, "bbv_umode_only", 0, [&](const UNUSED char *s) { bbv_umode_only = true; });
        parser.option(0, "warmup_size", 1, [&](const UNUSED char *s) { warmup_size = strtoul(s, nullptr, 10); });
        parser.option(0, "start_interval", 1, [&](const UNUSED char *s) { start_interval = strtoul(s, nullptr, 10); });
        parser.option(0, "max_intervals", 1, [&](const UNUSED char *s) { max_intervals = strtoul(s, nullptr, 10); });
        parser.option(0, "encode_bb_ids", 0, [&](const UNUSED char *s) { encode_bb_ids = true; });
    }

    void bbv_options_help() {
        #define E(s) fprintf(stderr,s)
        E("  ------------------------------------------------------------------------------\n");
        E("  BBV capture options\n");
        E("  ------------------------------------------------------------------------------\n");
        E("  --en_bbv              Enable BBV collection\n");
        E("  --bb_file=<path>      Base name of the file to dump. Name is appended with\n");
        E("                        _cpu<n> suffix depending on the core BB collection is\n");
        E("                        happening [default bbv.spike]\n");
        E("  --bbv_umode_only      SimPoint only user-mode instructions [default false]\n");
        E("  --simpoint_size=<n>   SimPoint window for BB collection [default 100,000,000]\n");
        E("  --warmup_size=<n>     Warmup window for BB collection, use with bbv_umode_only.\n");
        E("  --start_interval=<n>  Begin BBV tracing at interval n.\n");
        E("  --max_intervals=<n>   Trace n intervals.\n");
        E("  --encode_bb_ids       Use sequeuntial integers for BB IDs, instead of addresses [default false].\n");
        #undef E
    }
} // bb_tracer_options
