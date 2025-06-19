#include "bb_tracer.h"
#include "processor.h"
#include <iostream>

namespace bb_ctrl {
    simpoint_csr_t::simpoint_csr_t(processor_t *const proc, const reg_t addr, const reg_t init) :
            csr_t(proc, addr),
            val(init) {
    }

    bool simpoint_csr_t::unlogged_write(const reg_t value) noexcept {
        proc->get_bb_tracer().handle_simpoint_macro(proc->get_last_pc(), value);
        return true;
    }
} // namespace bb_ctrl

bb_tracer::bb_tracer(bool en_bbv, const std::string &bb_file_base_name,
                     uint64_t simpoint_size, uint32_t heart_id) : m_simpoint_size(simpoint_size),
                                                                  m_next_id(1),
                                                                  m_next_bbv_dump(simpoint_size),
                                                                  m_heart_id(heart_id),
                                                                  m_en_bbv(en_bbv) {
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
}


bb_tracer::~bb_tracer() {
    m_bb_file.close();
}

void bb_tracer::flush_bb_vector(const uint64_t steps) {
    static uint64_t instr_cnt{0};
    instr_cnt += steps;
    if (instr_cnt >= m_next_bbv_dump) {
        m_next_bbv_dump += m_simpoint_size;

        if (!m_bbv.empty()) {
            m_bb_file << "T";
            for (const auto& ent: m_bbv) {
                auto it = m_pc2id.find(ent.first);
                uint64_t id = 0u;
                if (it == m_pc2id.end()) {
                    id = m_next_id++;
                    m_pc2id[ent.first] = id;
                } else {
                    id = it->second;
                }

                m_bb_file << ":" << id << ":" << ent.second << " ";
            }
            m_bb_file << "\n";
            m_bb_file.flush();
            m_bbv.clear();
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
    if (m_last_pc && m_simpoint_roi) {
        m_total_insn_in_roi++;
        m_ninst++;
        capture_basic_block(pc);
        flush_bb_vector(steps);
    }
    if (m_simpoint_roi && pc != m_simpoint_en_pc) {
        m_last_pc = pc;
    }
    if(unlikely(m_terminate)){
        throw bb_ctrl::simpoint_terminate("Reached termination instruction in benchmark. Benchmark exited with code: " +
                                          std::to_string(m_benchmark_return_code) + "\n");
    }
}

bool bb_tracer::in_region_of_interest() const {return m_simpoint_roi;}

long int bb_tracer::get_total_insns() {
    return m_total_insn_in_roi;
}

void bb_tracer::handle_simpoint_macro(uint64_t pc, const reg_t val) noexcept {
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
            m_ninst++;
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
            m_total_insn_in_roi = 0;
        }
        std::cerr.flush();
    }
}


namespace bb_tracer_options {
    bool en_bbv = false;
    std::string bb_file{"bbv.spike"};
    uint64_t simpoint_size = 100000000UL;

    void set_options(option_parser_t &parser) {
        parser.option(0, "en_bbv", 0, [&](const char UNUSED *s) { en_bbv = true; });
        parser.option(0, "bb_file", 1,
                      [&](const char *s) { bb_tracer_options::bb_file = std::string(s); });
        parser.option(0, "simpoint_size", 1, [&](const char *s) { simpoint_size = strtoul(s, nullptr, 10); });
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
        E("  --simpoint_size=<n>   SimPoint window for BB collection [default 100,000,000]\n");
        #undef E
    }
} // bb_tracer_options
