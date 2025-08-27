#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "decode.h"
#include "csrs.h"
#include "fesvr/option_parser.h"

class processor_t;
namespace bb_ctrl {
    class simpoint_csr_t : public csr_t {
    public:
        simpoint_csr_t(processor_t *proc, reg_t addr, reg_t init);

        [[nodiscard]] reg_t read() const noexcept override {
            return 0;
        }

    protected:
        bool unlogged_write(reg_t val) noexcept override;

    private:
        reg_t val;
    };

    class simpoint_terminate : public std::runtime_error {
    public:
        explicit simpoint_terminate(const std::string &what)
                : std::runtime_error(what) {}
    };
}// namespace bb_ctrl

struct instr_track_t{
    uint64_t total_instr_count{0};
    uint64_t total_umode_instr_count{0};
    uint64_t roi_instr_count{0};
    uint64_t roi_umode_instr_count{0};
    reg_t pc{0};
};

class bb_tracer {
public:
    bb_tracer(processor_t* proc, bool en_bbv, const std::string &bb_file_base_name, uint64_t simpoint_size, uint64_t warmup_size, uint32_t heart_id);

    ~bb_tracer();

    void simpoint_step(uint64_t steps, uint64_t pc);

    void handle_simpoint_macro(uint64_t pc, reg_t val, uint64_t executed_insn_cnt) noexcept;

    bool in_region_of_interest() const;

    long int get_total_insns() const;

    uint64_t get_insn_count_on_roi_start() const;

    uint64_t get_benchmark_ppn() const;

private:
    int capture_basic_block(uint64_t pc);

    void flush_bb_vector(uint64_t steps);

    void log_simpoint_tracks(uint64_t pc);

    struct Simpoint {
        Simpoint(uint64_t i, int j) : start(i), id(j) {}

        bool operator<(const Simpoint &j) const { return (start < j.start); }

        uint64_t start;
        int id;
    };

    std::vector<Simpoint> simpoints;

    processor_t* m_proc;
    uint64_t m_simpoint_size{0};
    std::fstream m_bb_file;
    std::fstream m_bb_tracks_file;
    std::fstream m_simpoint_file;
    std::unordered_map<uint64_t, uint64_t> m_bbv;
    std::unordered_map<uint64_t, uint64_t> m_pc2id;
    int m_next_id{1};
    uint64_t m_ninst{0};
    uint64_t m_next_bbv_dump{0};
    uint32_t m_heart_id{0};
    bool m_simpoint_roi{false};
    bool m_en_bbv{false};
    bool m_terminate{false};
    uint64_t m_benchmark_return_code{0};
    uint64_t m_last_pc{0};
    uint64_t m_simpoint_en_pc{0};
    uint64_t m_total_insn_in_roi{0};
    uint64_t m_insn_num_roi_started{0};
    instr_track_t snippet_warmup_insn_track;
    instr_track_t next_snippet_warmup_insn_track;
    instr_track_t snippet_start_insn_track;
    instr_track_t snippet_end_insn_track;
    reg_t m_ppn{0};
    uint64_t m_warmup_size{0};
};

namespace bb_tracer_options {
    extern bool en_bbv;
    extern std::string bb_file;
    extern uint64_t simpoint_size;
    extern bool bbv_umode_only;
    extern uint64_t warmup_size;
    extern bool encode_bb_ids;

    void set_options(option_parser_t &parser);

    void bbv_options_help();
} // bb_tracer_options
