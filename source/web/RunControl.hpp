#pragma once

/*
 *  This file is part of the Avida Digital Evolution Research Platform, v5.0
 *  Copyright (C) 2026 Michigan State University & Dr. Charles Ofria
 *  Released under the MIT Public Licence.  See LICENSE.md for details.
 */

// Definitions included after AvidaWebApp; the web application uses one translation unit.

void AvidaWebApp::UpdateControls() {
  const bool worker_busy = SimulationWorkerBusy();
  const bool population_running = run_mode != RunMode::PAUSED || worker_busy;
  const bool complete = !worker_busy && Avida().IsComplete();
  restart_button.SetDisabled(!run_started || run_mode != RunMode::PAUSED || worker_busy);
  const bool run_target_reached = published_education_update.load(std::memory_order_acquire)
    >= education_target_update;
  UpdateEducationRunButton(
    population_running || run_target_reached
      || (!run_started && HasCompletedEducationCombination())
  );
  step_button.SetDisabled(complete || run_mode != RunMode::PAUSED || worker_busy);
  play_button.SetDisabled(complete);
  // Pause is safe and idempotent while paused; keeping it enabled preserves a
  // stable keyboard target for the transport controls.
  pause_button.SetDisabled(complete);
  fast_forward_button.SetDisabled(complete);
  color_selector.Disabled(complete || population_running);
  run_preset_selector.Disabled(run_started || population_running);
  experiment_preset_selector.Disabled(run_started || population_running);
  run_seed_input.Disabled(run_started || population_running);
  run_target_input.Disabled(population_running);
  org_stats_mode.SetDisabled(population_running);
  freezer_mode.SetDisabled(population_running);
  configure_mode.SetDisabled(population_running);

  play_button.SetAttr(
    "class",
    run_mode == RunMode::PLAY
      ? "transport-button icon-button is-active"
      : "transport-button icon-button"
  );
  pause_button.SetAttr(
    "class",
    run_mode == RunMode::PAUSED
      ? "transport-button icon-button is-active"
      : "transport-button icon-button"
  );
  fast_forward_button.SetAttr(
    "class",
    run_mode == RunMode::FAST_FORWARD
      ? "transport-button icon-button is-active"
      : "transport-button icon-button"
  );
  play_button.SetAttr("aria-pressed", run_mode == RunMode::PLAY ? "true" : "false");
  pause_button.SetAttr("aria-pressed", run_mode == RunMode::PAUSED ? "true" : "false");
  fast_forward_button.SetAttr(
    "aria-pressed",
    run_mode == RunMode::FAST_FORWARD ? "true" : "false"
  );
  save_organism_button.SetDisabled(population_running || !GetActiveOrganism());
  save_configuration_button.SetDisabled(population_running);
  save_run_button.SetDisabled(!run_started || population_running);
}

void AvidaWebApp::SetRunMode(RunMode new_mode) {
  if (new_mode != RunMode::PAUSED && !run_started) StartRun();
  const bool worker_busy = SimulationWorkerBusy();
  if (!worker_busy
      && new_mode != RunMode::PAUSED
      && Avida().ConsumePauseRequest()) {
    new_mode = RunMode::PAUSED;
  }
  if (!worker_busy && Avida().IsComplete()) new_mode = RunMode::PAUSED;
  run_mode = new_mode;
  simulation_pause_requested.store(
    run_mode == RunMode::PAUSED,
    std::memory_order_release
  );
  play_elapsed_ms = 0.0;

  // Update the button state before Start(), which immediately invokes the
  // animation callback and may spend a long time in fast-forward mode.
  UpdateControls();
  RefreshEducationStatus();
  if (!worker_busy) RefreshReadouts();

  if (run_mode == RunMode::PAUSED) {
    // The driver-owned update must reach its boundary before the animation can stop.
    if (!worker_busy && animation.GetActive()) animation.Stop();
  } else if (!animation.GetActive()) {
    animation.Start();
  }
}

void AvidaWebApp::ToggleRunMode(RunMode mode) {
  SetRunMode(run_mode == mode ? RunMode::PAUSED : mode);
}

void AvidaWebApp::FinishUpdate(bool can_continue,
                  bool redraw_population,
                  bool pause_requested) {
  if (!can_continue && Avida().IsExitPending()) {
    CaptureFinalView();
    Avida().Shutdown();
    redraw_population = false;
  }
  if (redraw_population && active_color_scale != ColorScale::BLANK) DrawPopulation();
  RefreshReadouts();
  pause_requested = Avida().ConsumePauseRequest() || pause_requested;
  if (Avida().GetUpdate() >= education_target_update) {
    FinalizeEducationRun("complete");
    pause_requested = true;
  } else if (!can_continue) {
    FinalizeEducationRun("finished");
  }
  if (!can_continue || pause_requested) SetRunMode(RunMode::PAUSED);
}

void AvidaWebApp::StepPopulation() {
  if (run_mode != RunMode::PAUSED) SetRunMode(RunMode::PAUSED);
  if (SimulationWorkerBusy() || Avida().IsComplete()
      || Avida().GetUpdate() >= education_target_update) return;
  if (!run_started) StartRun();
  (void) Avida().ConsumePauseRequest();  // An explicit step advances past a start-time pause.
  const bool can_continue = Avida().AdvanceUpdate();
  const size_t update = Avida().GetUpdate();
  if (update % education_sample_interval == 0 || update >= education_target_update) {
    const auto sample = CaptureEducationSample();
  (void) education_run_recorder.Append(sample);
  education_results_text.Redraw();
  UpdateEducationPlots();
  }
  published_education_update.store(update, std::memory_order_release);
  FinishUpdate(can_continue, true);
  UpdateControls();
}

void AvidaWebApp::RunEducationToTarget() {
  if (published_education_update.load(std::memory_order_acquire) >= education_target_update) {
    RestartPopulation();
    return;
  }
  const bool fresh_run = !run_started;
  SetRunMode(RunMode::FAST_FORWARD);
  // The first InitializePaused() can consume the engine's one-time startup
  // pause request and leave run_mode paused at update 0. Treat the button as a
  // single Run-to-target action by issuing the same fast-forward transition a
  // second time when that exact startup condition occurs.
  if (fresh_run && run_mode == RunMode::PAUSED && !SimulationWorkerBusy()
      && !Avida().IsComplete() && Avida().GetUpdate() < education_target_update) {
    SetRunMode(RunMode::FAST_FORWARD);
  }
}

std::map<emp::String, emp::String> AvidaWebApp::SnapshotSettingValues() const {
  std::map<emp::String, emp::String> values;
  const auto & settings = Avida().GetSettings();
  for (const emp::String & name : settings.GetSettingNames()) {
    values.emplace(name, settings.Get<emp::String>(name));
  }
  return values;
}

void AvidaWebApp::CreateConfiguredAvida(
  const std::map<emp::String, emp::String> & values,
  const emp::String & ancestor_genome
) {
  emp_assert(!SimulationWorkerBusy());
  if (animation.GetActive()) animation.Stop();
  if (avida) {
    Avida().GetPlugIn<WebInterfaceBridge>().SetOnStartCallback({});
    Avida().GetPlugIn<WebInterfaceBridge>().SetBeforeExitCallback({});
    avida.reset();
  }

  avida = std::make_unique<avida_t>();
  auto & settings = Avida().GetSettings();
  settings.Set("base.config_dir", std::string{"/config"});
  settings.Set("base.data_dir", std::string{"/data"});
  settings.Load(population_adapter_t::CONFIG_PATH);
  if (structured_config_initialized) {
    Reactions().SetConfigs(reaction_configs);
    Events().SetConfigs(event_configs);
  } else {
    reaction_configs = Reactions().GetConfigs();
    default_reaction_configs = reaction_configs;
    event_configs = Events().GetConfigs();
    default_event_configs = event_configs;
    structured_config_initialized = true;
  }
  for (const auto & [name, value] : values) {
    if (settings.HasSetting(name)) settings.Set(name, value);
  }
  if (values.empty()) {
    const auto & preset = avida_web::education::MUTATION_VARIATION;
    settings.Set("grid.width", preset.grid_width);
    settings.Set("grid.height", preset.grid_height);
    settings.Set("base.random_seed", preset.seeds[0]);
    settings.Set("mutations.substitution_prob", 0.0);
    education_seed = preset.seeds[0];
    if (!education_uses_quick_preset) {
      education_target_update = preset.target_update;
      education_sample_interval = preset.sample_interval;
    } else {
      education_target_update = avida_web::education::MUTATION_VARIATION_QUICK.target_update;
      education_sample_interval = avida_web::education::MUTATION_VARIATION_QUICK.sample_interval;
    }
  }
  configured_ancestor_genome = ancestor_genome;
  if (configured_ancestor_genome.size()) {
    static constexpr const char * frozen_ancestor_path = "/tmp/avida-web-frozen-ancestor.org";
    std::ofstream ancestor_output{frozen_ancestor_path};
    ancestor_output << configured_ancestor_genome;
    settings.Set("base.ancestor_filename", emp::String{frozen_ancestor_path});
  }

  run_mode = RunMode::PAUSED;
  run_started = false;
  has_final_snapshot = false;
  published_education_update.store(Avida().GetUpdate(), std::memory_order_release);
  last_grid_redraw_update = 0;
  play_elapsed_ms = 0.0;
  population_pixels.clear();
  continuous_values.clear();
  final_statistic_values.clear();
  final_color_legend_html.clear();
  active_organism = {};
  active_organism_name.clear();
  active_cell_id = avida_web::EMPTY_CELL;
  CollectPopulationViewOptions();
}

void AvidaWebApp::StartRun() {
  emp_assert(!run_started);
  ApplyStructuredConfiguration();
  Avida().GetPlugIn<DriverBuffered>().SetInjectAncestor(placed_organisms.empty());
  Avida().GetPlugIn<WebInterfaceBridge>().SetOnStartCallback(
    [this](){ ApplyPlacedOrganisms(); }
  );
  Avida().GetPlugIn<WebInterfaceBridge>().SetBeforeExitCallback({});
  Avida().InitializePaused();
  run_started = true;
  education_treatment_id = Avida().GetSettings().Get<double>("mutations.substitution_prob") >= 0.005
    ? "mutation-1" : "mutation-0";
  education_seed = Avida().GetSettings().Get<size_t>("base.random_seed");
  education_ancestor_sequence.clear();
  if (const auto ancestor = Organisms().LoadGenome("/config/ancestor.org")) {
    education_ancestor_sequence.reserve(ancestor->size());
    for (const auto instruction : *ancestor) {
      education_ancestor_sequence.push_back(static_cast<std::uint8_t>(instruction));
    }
  }
  education_run_recorder.Clear();
  pending_education_samples.clear();
  InitializeEducationRunRecord();
  const auto initial_sample = CaptureEducationSample();
  (void) education_run_recorder.Append(initial_sample);
  published_education_update.store(initial_sample.update, std::memory_order_release);
  CollectPopulationViewOptions();
  RequestInterfaceRebuild();
}

void AvidaWebApp::RestartPopulation() {
  if (!ConfirmPopulationRestart()) return;
  SetRunMode(RunMode::PAUSED);
  if (SimulationWorkerBusy()) return;
  const auto current_values = SnapshotSettingValues();
  if (run_started && !active_education_run_finalized) {
    FinalizeEducationRun("interrupted");
  }
  const emp::String current_ancestor = configured_ancestor_genome;
  CreateConfiguredAvida(current_values, current_ancestor);
  RequestInterfaceRebuild();
}

void AvidaWebApp::OnAnimationFrame(const UI::Animate & frame) {
  if (active_application_mode == ApplicationMode::ORGANISM) {
    if (organism_run_mode == OrganismRunMode::PAUSED) return;
    organism_play_elapsed_ms += frame.GetStepTime();
    const double interval = organism_run_mode == OrganismRunMode::PLAY
      ? ORGANISM_PLAY_INTERVAL_MS
      : ORGANISM_FAST_FORWARD_INTERVAL_MS;
    if (organism_play_elapsed_ms < interval) return;

    organism_play_elapsed_ms = 0.0;  // Do not catch up after a delayed frame.
    AdvancePlayingOrganismInstruction();
    return;
  }

  if (auto completed = ConsumeSimulationUpdate()) {
    CommitEducationSamples();
    const bool can_continue = *completed;
    const bool pause_requested = simulation_pause_requested.load(std::memory_order_acquire)
      || Avida().ConsumePauseRequest()
      || run_mode == RunMode::PAUSED;
    const bool publish_update = pause_requested
      || !can_continue
      || run_mode == RunMode::PLAY
      || Avida().GetUpdate() - last_grid_redraw_update >= FAST_FORWARD_REDRAW_UPDATES;
    if (publish_update) {
      const bool redraw_population = active_color_scale != ColorScale::BLANK;
      FinishUpdate(can_continue, redraw_population, pause_requested);
      if (active_color_scale == ColorScale::BLANK) {
        last_grid_redraw_update = Avida().GetUpdate();
      }
      // FinishUpdate() refreshes controls whenever the run actually pauses or completes.
      // Replacing unchanged buttons after every worker batch breaks hover state and can discard
      // a pointer-down before its matching click event arrives.
    }

    if (pending_checkpoint_import && run_mode == RunMode::PAUSED) {
      std::string checkpoint = std::move(*pending_checkpoint_import);
      pending_checkpoint_import.reset();
      interface_rebuild_requested = false;
      ImportCheckpointFile(checkpoint);
      return;
    }
    if (pending_education_import && run_mode == RunMode::PAUSED) {
      std::string archive = std::move(*pending_education_import);
      pending_education_import.reset();
      ImportEducationExperiment(archive);
      return;
    }
    if (pending_education_save && run_mode == RunMode::PAUSED) {
      SaveEducationExperiment();
      return;
    }
    if (interface_rebuild_requested) {
      interface_rebuild_requested = false;
      RequestInterfaceRebuild();
    }
  }

  if (run_mode == RunMode::PAUSED) {
    if (!SimulationWorkerBusy() && animation.GetActive()) animation.Stop();
    return;
  }

  if (SimulationWorkerBusy()) return;
  if (Avida().GetUpdate() >= education_target_update) {
    FinalizeEducationRun("complete");
    SetRunMode(RunMode::PAUSED);
    RefreshEducationStatus();
    return;
  }

  if (run_mode == RunMode::PLAY) {
    play_elapsed_ms += frame.GetStepTime();
    if (play_elapsed_ms < PLAY_INTERVAL_MS) return;
    play_elapsed_ms = 0.0;  // Do not catch up after a delayed or backgrounded frame.
  }
  const size_t requested_budget = run_mode == RunMode::FAST_FORWARD
    ? FAST_FORWARD_WORK_BATCH_UPDATES
    : 1;
  const size_t remaining = education_target_update - Avida().GetUpdate();
  const size_t update_budget = std::min(requested_budget, remaining);
  (void) DispatchSimulationUpdate(update_budget);
}

void AvidaWebApp::RefreshEducationStatus() {
  education_status_text.Redraw();
}

void AvidaWebApp::SetLocale(const std::string & locale_id) {
  if (locale_id == "es") {
    active_locale = avida::web::localization::Locale::SPANISH;
  } else if (locale_id == "qps-ploc") {
    active_locale = avida::web::localization::Locale::PSEUDO_EXPANDED;
  } else if (locale_id == "qps-plocm") {
    active_locale = avida::web::localization::Locale::PSEUDO_RTL;
  } else {
    active_locale = avida::web::localization::Locale::ENGLISH;
  }
  EM_ASM({
    document.documentElement.dir = $0 ? "rtl" : "ltr";
    document.documentElement.lang = UTF8ToString($1);
  }, active_locale == avida::web::localization::Locale::PSEUDO_RTL,
     locale_id.c_str());
  RequestInterfaceRebuild();
}

void AvidaWebApp::SetEducationTreatment(const std::string & treatment_id) {
  if (run_started || SimulationWorkerBusy()) return;
  const double probability = treatment_id == "one-percent" ? 0.01 : 0.0;
  education_treatment_id = treatment_id == "one-percent" ? "mutation-1" : "mutation-0";
  Avida().GetSettings().Set("mutations.substitution_prob", probability);
  run_mutation_input.Value(emp::MakeString(probability * 100.0));
  UpdateControls();
  RefreshEducationStatus();
}

void AvidaWebApp::SetEducationPreset(const std::string & preset_id) {
  if (run_started || SimulationWorkerBusy()) return;
  education_uses_quick_preset = preset_id == "mutation-variation-quick";
  const auto & preset = education_uses_quick_preset
    ? avida_web::education::MUTATION_VARIATION_QUICK
    : avida_web::education::MUTATION_VARIATION;
  education_target_update = preset.target_update;
  education_sample_interval = preset.sample_interval;
  run_target_input.Value(emp::MakeString(education_target_update));
  UpdateControls();
  RefreshEducationStatus();
}

void AvidaWebApp::SetEducationSeed(const std::string & seed_text) {
  if (run_started || SimulationWorkerBusy() || seed_text.empty()) return;
  char * end = nullptr;
  const unsigned long long parsed = std::strtoull(seed_text.c_str(), &end, 10);
  if (end == seed_text.c_str() || *end != '\0' || parsed == 0
      || parsed > std::numeric_limits<size_t>::max()) return;
  if (parsed != 42 && parsed != 43 && parsed != 44) return;
  education_seed = static_cast<size_t>(parsed);
  // Input::DoChange only updates the current DOM value. Store accepted edits
  // on the widget as well, or a later UI rebuild restores its old value.
  run_seed_input.Value(seed_text);
  Avida().GetSettings().Set("base.random_seed", education_seed);
  UpdateControls();
  RefreshEducationStatus();
}

void AvidaWebApp::Initialize() {
  InstallCheckpointWarningCapture();
  CreateConfiguredAvida();
  StartSimulationWorker();
  default_setting_values = SnapshotSettingValues();
  InitializeFreezer();
  InitializeDragCallbacks();
  InstallOrganismDragBridge(drop_organism_callback_id, freeze_grid_callback_id);
  InstallPopulationKeyboardBridge(
    population_step_callback_id, population_fast_forward_callback_id
  );
  InstallOrganismKeyboardBridge(organism_space_step_callback_id);
  InstallOrganismModeInteractionBridge(
    organism_scrub_callback_id, organism_head_callback_id
  );
  std::println(
    "Loaded {} (substitution probability = {}).",
    population_adapter_t::CONFIG_PATH,
    Avida().GetSettings().Get<double>("mutations.substitution_prob")
  );
  RebuildInterface();
}
