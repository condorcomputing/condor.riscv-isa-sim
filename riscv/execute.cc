// See LICENSE for license details.

#include "config.h"
#include "processor.h"
#include "mmu.h"
#include "disasm.h"
#include "decode_macros.h"
#include "encoding.h"
#include "stf_handler.h"
#include <cassert>

static void commit_log_reset(processor_t* p)
{
  p->get_state()->log_reg_write.clear();
  p->get_state()->log_mem_read.clear();
  p->get_state()->log_mem_write.clear();
}

static void commit_log_stash_privilege(processor_t* p)
{
  state_t* state = p->get_state();
  state->last_inst_priv = state->prv;
  state->last_inst_xlen = p->get_xlen();
  state->last_inst_flen = p->get_flen();
}

static void commit_log_print_value(FILE *log_file, int width, const void *data)
{
  assert(log_file);

  switch (width) {
    case 8:
      fprintf(log_file, "0x%02" PRIx8, *(const uint8_t *)data);
      break;
    case 16:
      fprintf(log_file, "0x%04" PRIx16, *(const uint16_t *)data);
      break;
    case 32:
      fprintf(log_file, "0x%08" PRIx32, *(const uint32_t *)data);
      break;
    case 64:
      fprintf(log_file, "0x%016" PRIx64, *(const uint64_t *)data);
      break;
    default:
      if (width % 8 == 0) {
        const uint8_t *arr = (const uint8_t *)data;

        fprintf(log_file, "0x");
        for (int idx = width / 8 - 1; idx >= 0; --idx) {
          fprintf(log_file, "%02" PRIx8, arr[idx]);
        }
      } else {
        abort();
      }
      break;
  }
}

static void commit_log_print_value(FILE *log_file, int width, uint64_t val)
{
  commit_log_print_value(log_file, width, &val);
}

static void commit_log_print_insn(processor_t *p, reg_t pc, insn_t insn)
{
  FILE *log_file = p->get_log_file();

  auto& reg = p->get_state()->log_reg_write;
  auto& load = p->get_state()->log_mem_read;
  auto& store = p->get_state()->log_mem_write;
  int priv = p->get_state()->last_inst_priv;
  int xlen = p->get_state()->last_inst_xlen;
  int flen = p->get_state()->last_inst_flen;

  // print core id on all lines so it is easy to grep
  fprintf(log_file, "core%4" PRId32 ": ", p->get_id());

  fprintf(log_file, "%1d ", priv);
  commit_log_print_value(log_file, xlen, pc);
  fprintf(log_file, " (");
  commit_log_print_value(log_file, insn.length() * 8, insn.bits());
  fprintf(log_file, ")");
  bool show_vec = false;

  for (auto item : reg) {
    if (item.first == 0)
      continue;

    char prefix = ' ';
    int size=0;
    int rd = item.first >> 4;
    bool is_vec = false;
    bool is_vreg = false;
    switch (item.first & 0xf) {
    case 0:
      size = xlen;
      prefix = 'x';
      break;
    case 1:
      size = flen;
      prefix = 'f';
      break;
    case 2:
      size = p->VU.VLEN;
      prefix = 'v';
      is_vreg = true;
      break;
    case 3:
      is_vec = true;
      break;
    case 4:
      size = xlen;
      prefix = 'c';
      break;
    default:
      assert("can't been here" && 0);
      break;
    }

    if (!show_vec && (is_vreg || is_vec)) {
        fprintf(log_file, " e%ld %s%ld l%ld",
                (long)p->VU.vsew,
                p->VU.vflmul < 1 ? "mf" : "m",
                p->VU.vflmul < 1 ? (long)(1 / p->VU.vflmul) : (long)p->VU.vflmul,
                (long)p->VU.vl->read());
        show_vec = true;
    }

    if (!is_vec) {
      if (prefix == 'c')
        fprintf(log_file, " c%d_%s ", rd, csr_name(rd));
      else
        fprintf(log_file, " %c%-2d ", prefix, rd);
      if (is_vreg)
        commit_log_print_value(log_file, size, &p->VU.elt<uint8_t>(rd, 0));
      else
        commit_log_print_value(log_file, size, item.second.v);
    }
  }

  for (auto item : load) {
    fprintf(log_file, " mem ");
    commit_log_print_value(log_file, xlen, std::get<0>(item));
  }

  for (auto item : store) {
    fprintf(log_file, " mem ");
    commit_log_print_value(log_file, xlen, std::get<0>(item));
    fprintf(log_file, " ");
    commit_log_print_value(log_file, std::get<2>(item) << 3, std::get<1>(item));
  }
  fprintf(log_file, "\n");
}

inline void processor_t::update_histogram(reg_t pc)
{
  if (histogram_enabled)
    pc_histogram[pc]++;
}

void processor_t::maybe_checkpoint_interval(reg_t pc, reg_t npc, reg_t instret, reg_t steps_remaining) {
   uint64_t executed_insns = stfhandler->get_executed_roi_umode_insns();

   if (executed_insns && checkpoint_interval && (executed_insns % checkpoint_interval == 0)) {
     this->steps_remaining = steps_remaining;

     reg_t stash_minstret =  state.minstret->val;
     reg_t stash_mcycle = state.mcycle->val;
     if (!(state.mcountinhibit->read() & MCOUNTINHIBIT_IR))
       state.minstret->val += instret;
     if (!(state.mcountinhibit->read() & MCOUNTINHIBIT_CY))
       state.mcycle->val += instret;

     reg_t stash_pc = get_state()->pc;
     get_state()->pc = npc;

     reg_t interval_num = std::ceil(executed_insns / bb_tracer_options::simpoint_size);

     std::string tag = std::to_string(interval_num) + "_" + std::to_string(get_executed_insns());

     sim->checkpoint(tag);

     get_state()->pc = stash_pc; // or pc?
     state.minstret->val = stash_minstret;
     state.mcycle->val = stash_mcycle;
   }

   if (executed_insns && checkpoint_interval && !bb_tracer_options::en_bbv) {
      if (executed_insns % bb_tracer_options::simpoint_size == (bb_tracer_options::simpoint_size - bb_tracer_options::warmup_size + 1)) {
        m_bb_tracer.log_simpoint_warmup_insn_track(pc);
      } else if (executed_insns % bb_tracer_options::simpoint_size == 1) {
        m_bb_tracer.log_simpoint_start_track(pc);
      }

      if (executed_insns % checkpoint_interval == 0) {
        m_bb_tracer.log_simpoint_end_insn_track(pc);
      }
   }
}

// These two functions are expected to be inlined by the compiler separately in
// the processor_t::step() loop. The logged variant is used in the slow path
static inline reg_t execute_insn_fast(processor_t* p, reg_t pc, insn_fetch_t fetch) {
  return fetch.func(p, fetch.insn, pc);
}
static inline reg_t execute_insn_logged(processor_t* p, state_t* state, reg_t pc, insn_fetch_t fetch)
{
  if (p->get_log_commits_enabled() || stfhandler->stf_enable_log_commits()) {
    commit_log_reset(p);
  }
  commit_log_stash_privilege(p);

  reg_t npc_or_serialize_flag;
  reg_t npc;

  try {
    npc_or_serialize_flag = fetch.func(p, fetch.insn, pc);

    if (npc_or_serialize_flag != PC_SERIALIZE_BEFORE) {
      // "next_pc" is in state.pc; see decode_macros.h
      if (npc_or_serialize_flag == PC_SERIALIZE_AFTER) {
        npc = p->get_state()->pc;
      } else {
        npc = npc_or_serialize_flag;
      }

      stfhandler->trace_insn(p,fetch,pc,npc,"SLOW LOOP");
      if ((state->prv_changed ? state->prev_prv : state->prv) == 0 || !bb_tracer_options::bbv_umode_only) {
        p->get_bb_tracer().simpoint_step(1u, pc);
      }

      if (p->get_log_commits_enabled()) {
        commit_log_print_insn(p, pc, fetch.insn);
      }
     }
  } catch (wait_for_interrupt_t &t) {
      if (p->get_log_commits_enabled()) {
        commit_log_print_insn(p, pc, fetch.insn);
      }
      throw;
  } catch(mem_trap_t& t) {
      //handle segfault in middle of vector load/store
      if (p->get_log_commits_enabled()) {
        for (auto item : p->get_state()->log_reg_write) {
          if ((item.first & 3) == 3) {
            commit_log_print_insn(p, pc, fetch.insn);
            break;
          }
        }
      }
      throw;
  } catch(...) {
    throw;
  }
  p->update_histogram(pc);

  return npc_or_serialize_flag;
}

bool processor_t::slow_path() 
{
  return debug || state.single_step != state.STEP_NONE || state.debug_mode ||
         log_commits_enabled || histogram_enabled || in_wfi || 
         check_triggers_icount || stfhandler->in_traceable_region() || get_bb_tracer().in_region_of_interest();
}

// fetch/decode/execute loop
void processor_t::step(size_t n)
{
  mmu_t* _mmu = mmu;

  if (!state.debug_mode) {
    if (halt_request == HR_REGULAR) {
      enter_debug_mode(DCSR_CAUSE_DEBUGINT, 0);
    } else if (halt_request == HR_GROUP) {
      enter_debug_mode(DCSR_CAUSE_GROUP, 0);
    } else if (halt_on_reset) {
      halt_on_reset = false;
      enter_debug_mode(DCSR_CAUSE_HALT, 0);
    }
  }

  if (extension_enabled(EXT_ZICCID)) {
    // Ziccid requires stores eventually become visible to instruction fetch,
    // so periodically flush the I$
    if (ziccid_flush_count-- == 0) {
      ziccid_flush_count += ZICCID_FLUSH_PERIOD;
      _mmu->flush_icache();
    }
  }

  while (n > 0) {
    reg_t instret = 0;
    reg_t pc = state.pc;
    reg_t ppc = state.pc;
    mmu_t* _mmu = mmu;

    if (!checkpoint_restored) {
      state.prv_changed = false;
      state.v_changed = false;
    } else { checkpoint_restored = false; }
    insn_fetch_t fetch;

    #define advance_pc() \
      if (unlikely(invalid_pc(pc))) { \
        switch (pc) { \
          case PC_SERIALIZE_BEFORE: state.serialized = true; break; \
          case PC_SERIALIZE_AFTER: ++instret; break; \
          default: abort(); \
        } \
        pc = state.pc; \
        break; \
      } else { \
        state.pc = pc; \
        instret++; \
      }

    try
    {
      take_pending_interrupt();

      check_if_lpad_required();

      if (unlikely(slow_path()))
      {
        // Main simulation loop, slow path.
        while (instret < n)
        {
          if (unlikely(!state.serialized && state.single_step == state.STEP_STEPPED)) {
            state.single_step = state.STEP_NONE;
            if (!state.debug_mode) {
              enter_debug_mode(DCSR_CAUSE_STEP, 0);
              // enter_debug_mode changed state.pc, so we can't just continue.
              break;
            }
          }

          if (unlikely(state.single_step == state.STEP_STEPPING)) {
            state.single_step = state.STEP_STEPPED;
          }

          if (!state.serialized && check_triggers_icount) {
            auto match = TM.detect_icount_match();
            if (match.has_value()) {
              assert(match->timing == triggers::TIMING_BEFORE);
              throw triggers::matched_t((triggers::operation_t)0, 0, match->action, state.v);
            }
          }

          // debug mode wfis must nop
          if (unlikely(in_wfi && !state.debug_mode)) {
            throw wait_for_interrupt_t();
          }

          in_wfi = false;
          fetch = mmu->load_insn(pc);
          if (debug && !state.serialized)
            disasm(fetch.insn);
          ppc = pc;
          pc = execute_insn_logged(this, &state, pc, fetch);
          if (pc != PC_SERIALIZE_BEFORE) {
            maybe_checkpoint_interval(ppc, pc, instret, (n)-(instret+1));
            stfhandler->incr_executed_instructions(this);
          }
          advance_pc();

          // Resume from debug mode in critical error
          if (state.critical_error && !state.debug_mode) {
            if (state.dcsr->read() & DCSR_CETRIG) {
              enter_debug_mode(DCSR_CAUSE_EXTCAUSE, DCSR_EXTCAUSE_CRITERR);
            } else {
              // Handling of critical error is implementation defined
              // For now just enter debug mode
              enter_debug_mode(DCSR_CAUSE_HALT, 0);
            }
          }
        }
      }
      else while (instret < n)
      {
        //This check should never fire 
        if(unlikely(stfhandler->in_traceable_region())) {
          fprintf(stderr,"-E: unexpected trace region while in fast loop\n");
          assert(0);
        }

        // Main simulation loop, fast path.
        for (auto ic_entry = _mmu->access_icache(pc); ; ) {
          fetch = ic_entry->data;
          //If this is the start macro we exit this loop and process 
          //in the slow loop
          if(unlikely(stfhandler->is_start_of_region(fetch.insn.bits()))) {
            break; //exit the for(;;) before insn is executed
          }
          ppc = pc;
          pc = execute_insn_fast(this, pc, fetch);
          ic_entry = ic_entry->next;
          if (unlikely(ic_entry->tag != pc))
            break;
          if (unlikely(instret + 1 == n))
            break;
          instret++;
          maybe_checkpoint_interval(ppc, pc, instret, (n)-(instret));
          stfhandler->incr_executed_instructions(this);
          state.pc = pc;
        }

        // Detect if we entered the trace region before executing current "pc":
        if(unlikely(stfhandler->in_traceable_region())) {
          break; //exit while
        }
        if (pc != PC_SERIALIZE_BEFORE) {
          // "pc" was executed, so increment count
          maybe_checkpoint_interval(ppc, pc, instret, (n)-(instret+1));
          stfhandler->incr_executed_instructions(this);
        }
        advance_pc();
        // Detect if the new PC in in the trace region (note this one calls "is_start_of_region()?")
        if(unlikely(m_bb_tracer.in_region_of_interest() || stfhandler->is_start_of_region(0))) {
          break; //exit while
        }
      }
    }
    catch(trap_t& t)
    {
      take_trap(t, pc);

      stfhandler->trace_event(this,fetch,pc,get_state()->pc,t,"TRAP");

      if (m_bb_tracer.in_region_of_interest() && t.cause() == CAUSE_USER_ECALL && bb_tracer_options::bbv_umode_only) {
         // BBV trace usermode ecalls to keep instruction counts consistent with STF trace
         m_bb_tracer.simpoint_step(1u, pc);
      }

      n = instret;

      // If critical error then enter debug mode critical error trigger enabled
      if (state.critical_error) {
        if (state.dcsr->read() & DCSR_CETRIG) {
          enter_debug_mode(DCSR_CAUSE_EXTCAUSE, DCSR_EXTCAUSE_CRITERR);
        } else {
          // Handling of critical error is implementation defined
          // For now just enter debug mode
          enter_debug_mode(DCSR_CAUSE_HALT, 0);
        }
      }
      // Trigger action takes priority over single step
      auto match = TM.detect_trap_match(t);
      if (match.has_value())
        take_trigger_action(match->action, 0, state.pc, 0);
      else if (unlikely(state.single_step == state.STEP_STEPPED)) {
        state.single_step = state.STEP_NONE;
        enter_debug_mode(DCSR_CAUSE_STEP, 0);
      }
    }
    catch (triggers::matched_t& t)
    {
      take_trigger_action(t.action, t.address, pc, t.gva);
    }
    catch(trap_debug_mode&)
    {
      enter_debug_mode(DCSR_CAUSE_SWBP, 0);
    }
    catch (wait_for_interrupt_t &t)
    {
      // Return to the outer simulation loop, which gives other devices/harts a
      // chance to generate interrupts.
      //
      // In the debug ROM this prevents us from wasting time looping, but also
      // allows us to switch to other threads only once per idle loop in case
      // there is activity.
      n = ++instret;
      maybe_checkpoint_interval(ppc, pc, instret, 0);
      stfhandler->incr_executed_instructions(this);
      in_wfi = true;
    }
    catch(stf_trace_complete &e) {
      if (!(state.mcountinhibit->read() & MCOUNTINHIBIT_IR))
        state.minstret->bump(instret);
      if (!(state.mcountinhibit->read() & MCOUNTINHIBIT_CY))
        state.mcycle->bump(instret);
      throw;
    }

    state.minstret->bump((state.mcountinhibit->read() & MCOUNTINHIBIT_IR) ? 0 : instret);

    // Model a hart whose CPI is 1.
    state.mcycle->bump((state.mcountinhibit->read() & MCOUNTINHIBIT_CY) ? 0 : instret);

    n -= instret;
  }
}
