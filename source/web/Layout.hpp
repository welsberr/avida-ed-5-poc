#pragma once

/*
 *  This file is part of the Avida Digital Evolution Research Platform, v5.0
 *  Copyright (C) 2026 Michigan State University & Dr. Charles Ofria
 *  Released under the MIT Public Licence.  See LICENSE.md for details.
 */

// Definitions included after AvidaWebApp; the web application uses one translation unit.

void AvidaWebApp::RequestInterfaceRebuild() {
  if (SimulationWorkerBusy()) {
    interface_rebuild_requested = true;
    return;
  }
  if (interface_rebuild_requested) return;
  interface_rebuild_requested = true;
  emp::DelayCall([this](){
    if (!interface_rebuild_requested) return;
    if (SimulationWorkerBusy()) return;
    interface_rebuild_requested = false;
    RebuildInterface();
  }, 0);
}

void AvidaWebApp::SetSidePanel(SidePanel panel) {
  active_side_panel = panel;
  const bool show_population = panel == SidePanel::POPULATION || panel == SidePanel::FREEZER;
  // Keep cell inspection visible with the lesson metrics when the population tab is active.
  const bool show_organism = panel == SidePanel::ORGANISM
    || panel == SidePanel::POPULATION || panel == SidePanel::FREEZER;
  const bool show_freezer = panel == SidePanel::FREEZER;
  const bool show_configuration = panel == SidePanel::CONFIGURATION;

  run_inspector.SetCSS("display", show_population ? "block" : "none");
  org_stats_inspector.SetCSS("display", show_organism ? "block" : "none");
  // The Freezer follows the legacy Avida-ED workspace: it stays below the
  // Viewer Chooser in the left rail while the Lab Bench remains visible.
  freezer_inspector.SetCSS("display", "block");
  configuration_inspector.SetCSS("display", show_configuration ? "block" : "none");
  pop_stats_mode.SetAttr(
    "class", show_population
      ? "mode-button side-mode-button is-active" : "mode-button side-mode-button"
  );
  org_stats_mode.SetAttr(
    "class", show_organism
      ? "mode-button side-mode-button is-active" : "mode-button side-mode-button"
  );
  freezer_mode.SetAttr(
    "class", show_freezer
      ? "mode-button side-mode-button is-active" : "mode-button side-mode-button"
  );
  configure_mode.SetAttr(
    "class", show_configuration
      ? "mode-button side-mode-button is-active" : "mode-button side-mode-button"
  );
  pop_stats_mode.SetAttr("aria-pressed", show_population ? "true" : "false");
  org_stats_mode.SetAttr("aria-pressed", show_organism ? "true" : "false");
  freezer_mode.SetAttr("aria-pressed", show_freezer ? "true" : "false");
  configure_mode.SetAttr("aria-pressed", show_configuration ? "true" : "false");
  if (show_organism) org_stats_content.Redraw();
}

void AvidaWebApp::BuildInterface() {
  UI::Div app{"avida_app"};
  app.AddAttr("class", "avida-app education-app");

  UI::Div header{"app_header"};
  header.AddAttr("class", "app-header");

  UI::Div primary_header{"primary_header"};
  primary_header.AddAttr("class", "primary-header");

  UI::Div brand{"brand"};
  brand.AddAttr("class", "brand");
  UI::Image logo{"assets/legacy-ed4/Avida-ED-logo.png", "avida_logo"};
  logo.Alt("Avida-ED").AddAttr("class", "brand-logo");
  brand << logo;

  // This selector is a persistent widget member while the interface is rebuilt. Use fixed option
  // IDs so each build replaces the four labels and callbacks instead of appending duplicates.
  language_selector.SetOption(PresentationText("English").str(), [this](){ SetLocale("en"); }, 0);
  language_selector.SetOption(PresentationText("Español").str(), [this](){ SetLocale("es"); }, 1);
  language_selector.SetOption(
    PresentationText("Pseudo: expanded strings").str(), [this](){ SetLocale("qps-ploc"); }, 2
  );
  language_selector.SetOption(
    PresentationText("Pseudo: right-to-left layout").str(),
    [this](){ SetLocale("qps-plocm"); }, 3
  );
  language_selector.SelectID(
    active_locale == avida::web::localization::Locale::ENGLISH ? 0
      : active_locale == avida::web::localization::Locale::SPANISH ? 1
        : active_locale == avida::web::localization::Locale::PSEUDO_EXPANDED ? 2 : 3
  );
  language_selector.SetAttr("aria-label", Localized(avida::web::localization::key::LANGUAGE_LABEL));
  language_selector.AddAttr("class", "language-selector");

  UI::Div product_identity{"product_identity"};
  product_identity.AddAttr("class", "product-identity");
  product_identity << emp::MakeString(
    "<p class='product-title'>", emp::MakeWebSafe(Localized(avida::web::localization::key::APP_TITLE)),
    " · ", emp::MakeWebSafe(PresentationText("Mutation and genetic variation")),
    "</p><p class='product-version'>", emp::MakeWebSafe(PresentationText("Avida 5 education preview")), "</p>"
  );

  UI::Div modes{"mode_buttons"};
  modes.AddAttr("class", "mode-buttons");
  modes << emp::MakeString(
    "<h2 class='viewer-chooser-heading'>",
    emp::MakeWebSafe(PresentationText("Viewer chooser")),
    "</h2>"
  );
  UI::Button population_mode{
    [this](){ SetApplicationMode(ApplicationMode::POPULATION); },
    emp::MakeString("<img src='assets/legacy-ed4/Avida-ED-population-icon.png' alt=''><span>",
      emp::MakeWebSafe(PresentationText("Population")), "</span>"),
    "population-mode"
  };
  UI::Button organism_mode{
    [this](){ SetApplicationMode(ApplicationMode::ORGANISM); },
    emp::MakeString("<img src='assets/legacy-ed4/Avida-ED-organism-icon.png' alt=''><span>",
      emp::MakeWebSafe(Localized(avida::web::localization::key::MODE_SEQUENCES)), "</span>"),
    "organism_mode"
  };
  UI::Button analyze_mode{
    [this](){ SetApplicationMode(ApplicationMode::COMPARE); },
    emp::MakeString("<img src='assets/legacy-ed4/Avida-ED-analysis-icon.png' alt=''><span>",
      emp::MakeWebSafe(Localized(avida::web::localization::key::MODE_COMPARE)), "</span>"),
    "compare-mode"
  };
  const bool population_active = active_application_mode == ApplicationMode::POPULATION;
  const bool organism_active = active_application_mode == ApplicationMode::ORGANISM;
  const bool compare_active = active_application_mode == ApplicationMode::COMPARE;
  population_mode.AddAttr("class", population_active ? "mode-button is-active" : "mode-button");
  organism_mode.AddAttr("class", organism_active ? "mode-button is-active" : "mode-button");
  analyze_mode.AddAttr("class", compare_active ? "mode-button is-active" : "mode-button");
  analyze_mode.SetDisabled(std::count_if(
    completed_education_runs.begin(), completed_education_runs.end(),
    [](const auto & run){ return run.status == "complete"; }
  ) < 2);
  population_mode.SetAttr("aria-label", PresentationText("Population Mode"));
  organism_mode.SetAttr("aria-label", PresentationText("Organism Mode"));
  analyze_mode.SetAttr("aria-label", PresentationText("Compare saved runs"));
  population_mode.SetAttr("aria-pressed", population_active ? "true" : "false");
  organism_mode.SetAttr("aria-pressed", organism_active ? "true" : "false");
  analyze_mode.SetAttr("aria-pressed", compare_active ? "true" : "false");
  population_mode.SetTitle(PresentationText("Population Mode"));
  organism_mode.SetTitle(PresentationText("Organism Mode"));
  analyze_mode.SetTitle(completed_education_runs.size() < 2
    ? PresentationText("Complete two runs to compare them.")
    : PresentationText("Compare the completed runs."));
  modes << population_mode;
  modes << organism_mode;
  modes << analyze_mode;

  UI::Div side_modes{"side_mode_buttons"};
  side_modes.AddAttr("class", "mode-buttons side-mode-buttons");
  if (!population_active) side_modes.SetCSS("visibility", "hidden");
  pop_stats_mode = UI::Button{
    [this](){ SetSidePanel(SidePanel::POPULATION); },
    emp::MakeString("<img src='assets/icons/StatsPop.png' alt=''><span>",
      emp::MakeWebSafe(Localized("shell.sidepanel.run")), "</span>"),
    "pop_stats_mode"
  };
  org_stats_mode = UI::Button{
    [this](){ SetSidePanel(SidePanel::ORGANISM); },
    emp::MakeString("<img src='assets/icons/StatsOrg.png' alt=''><span>",
      emp::MakeWebSafe(PresentationText("Cell")), "</span>"),
    "org_stats_mode"
  };
  freezer_mode = UI::Button{
    [this](){ SetSidePanel(SidePanel::FREEZER); },
    emp::MakeString("<img src='assets/icons/Freezer.png' alt=''><span>",
      emp::MakeWebSafe(PresentationText("Freezer")), "</span>"),
    "freezer_mode"
  };
  configure_mode = UI::Button{
    [this](){
      SetSidePanel(
        active_side_panel == SidePanel::CONFIGURATION
          ? SidePanel::POPULATION
          : SidePanel::CONFIGURATION
      );
    },
    emp::MakeString("<img src='assets/icons/Config.png' alt=''><span>",
      emp::MakeWebSafe(PresentationText("Setup")), "</span>"),
    "configure_mode"
  };
  pop_stats_mode.AddAttr("class", "mode-button side-mode-button is-active");
  org_stats_mode.AddAttr("class", "mode-button side-mode-button");
  freezer_mode.AddAttr("class", "mode-button side-mode-button");
  configure_mode.AddAttr("class", "mode-button side-mode-button");
  configure_mode.SetDisabled(true);
  pop_stats_mode.SetAttr("aria-label", PresentationText("Population Statistics"));
  org_stats_mode.SetAttr("aria-label", PresentationText("Organism Statistics"));
  freezer_mode.SetAttr("aria-label", PresentationText("Freezer"));
  configure_mode.SetAttr("aria-label", PresentationText("Configure"));
  pop_stats_mode.SetAttr("aria-controls", "run_inspector");
  org_stats_mode.SetAttr("aria-controls", "org_stats_inspector");
  freezer_mode.SetAttr("aria-controls", "freezer_inspector");
  configure_mode.SetAttr("aria-controls", "configuration_inspector");
  pop_stats_mode.SetAttr("aria-pressed", "true");
  org_stats_mode.SetAttr("aria-pressed", "false");
  freezer_mode.SetAttr("aria-pressed", "false");
  configure_mode.SetAttr("aria-pressed", "false");
  pop_stats_mode.SetTitle(PresentationText("Population Statistics"));
  org_stats_mode.SetTitle(PresentationText("Organism Statistics"));
  freezer_mode.SetTitle(PresentationText("Freezer"));
  configure_mode.SetTitle(PresentationText(
    "The lesson holds other settings fixed; see Experiment setup for the controlled values."
  ));
  side_modes << pop_stats_mode;
  side_modes << org_stats_mode;
  side_modes << freezer_mode;
  side_modes << configure_mode;

  primary_header << brand;
  primary_header << product_identity;
  primary_header << language_selector;
  header << primary_header;
  header << side_modes;
  app << header;

  UI::Div main_region{"main_region"};
  main_region.AddAttr("class", "main-region");
  UI::Div legacy_sidebar{"legacy_sidebar"};
  legacy_sidebar.AddAttr("class", "legacy-sidebar");
  modes.SetAttr("aria-label", PresentationText("Viewer chooser"));
  legacy_sidebar << modes;

  if (!population_active) {
    if (organism_active) BuildOrganismModeWorkspace(main_region);
    else BuildCompareModeWorkspace(main_region);
    main_region << legacy_sidebar;
    app << main_region;
    document << app;
    return;
  }

  UI::Div workspace{"workspace"};
  workspace.AddAttr("class", "workspace");
  UI::Div population_card{"population_card"};
  population_card.AddAttr("class", "population-card");
  population_card << emp::MakeString(
    "<div class='population-heading'><img src='assets/legacy-ed4/Avida-ED-dish-icon.png' ",
    "alt='' aria-hidden='true'><p class='population-description'>",
    population_adapter_t::DESCRIPTION, "</p></div>"
  );

  UI::Div canvas_frame{"canvas_frame"};
  canvas_frame.AddAttr("class", "canvas-frame");
  UI::Canvas population_canvas{
    static_cast<double>(PopulationWidth()),
    static_cast<double>(PopulationHeight()),
    "population_canvas"
  };
  population_canvas.AddAttr("class", "population-canvas");
  population_canvas.SetAttr("role", "img");
  population_canvas.SetAttr("aria-label", population_adapter_t::DESCRIPTION);
  population_canvas.SetAttr("tabindex", "0");
  population_canvas.SetAttr("data-grid-width", PopulationWidth());
  population_canvas.SetAttr("data-grid-height", PopulationHeight());
  population_canvas.SetTitle(PresentationText(population_adapter_t::PLACEMENT_HELP));
  population_canvas.SetAttr("oncontextmenu", "event.preventDefault();");
  population_canvas.OnKeydown(std::function<void(UI::KeyboardEvent)>{
    [this](UI::KeyboardEvent event) {
      size_t cell = active_cell_id == avida_web::EMPTY_CELL ? 55 : active_cell_id;
      const size_t row = cell / PopulationWidth();
      const size_t column = cell % PopulationWidth();
      if (event.keyCode == 37 && column > 0) --cell;
      else if (event.keyCode == 39 && column + 1 < PopulationWidth()) ++cell;
      else if (event.keyCode == 38 && row > 0) cell -= PopulationWidth();
      else if (event.keyCode == 40 && row + 1 < PopulationHeight()) cell += PopulationWidth();
      else return;
      SelectPopulationCell(cell);
    }
  });
  population_canvas.OnClick(std::function<void(UI::MouseEvent)>{
    [this](UI::MouseEvent event) {
      const int cell_id = GetPopulationCellAtClient(
        event.clientX,
        event.clientY,
        static_cast<int>(PopulationWidth()),
        static_cast<int>(PopulationHeight())
      );
      if (cell_id < 0) return;
      if (event.ctrlKey) {
        OpenGridCellMenu(
          static_cast<size_t>(cell_id), event.clientX, event.clientY
        );
      } else {
        SelectPopulationCell(static_cast<size_t>(cell_id));
      }
    }
  });
  population_canvas.On("contextmenu", std::function<void(UI::MouseEvent)>{
    [this](UI::MouseEvent event) {
      const int cell_id = GetPopulationCellAtClient(
        event.clientX,
        event.clientY,
        static_cast<int>(PopulationWidth()),
        static_cast<int>(PopulationHeight())
      );
      if (cell_id >= 0) {
        OpenGridCellMenu(
          static_cast<size_t>(cell_id), event.clientX, event.clientY
        );
      }
    }
  });
  UI::Div population_surface{"population_grid_surface"};
  population_surface.AddAttr("class", "population-grid-surface");
  population_surface.AddAttr(
    "style",
    emp::MakeString(
      "--grid-cell-width: ", 100.0 / PopulationWidth(),
      "%; --grid-cell-height: ", 100.0 / PopulationHeight(), "%;"
    )
  );
  population_surface << population_canvas;
  UI::Div active_cell_highlight{"active_cell_highlight"};
  active_cell_highlight.AddAttr("class", "active-cell-highlight");
  active_cell_highlight.SetAttr("aria-hidden", "true");
  population_surface << active_cell_highlight;
  UI::Div grid_cell_menu{"grid_cell_menu"};
  grid_cell_menu.AddAttr("class", "grid-cell-menu");
  grid_cell_menu.SetAttr("role", "menu");
  grid_cell_menu.SetAttr("aria-label", PresentationText("Organism actions"));
  grid_cell_menu.SetCSS("display", "none");
  UI::Button save_grid_organism_button{
    [this](){
      HideGridCellMenu();
      SaveGridCellOrganism(grid_context_cell_id);
    },
    PresentationText("Save Organism").str(),
    "save_grid_organism_button"
  };
  save_grid_organism_button.AddAttr("class", "grid-cell-menu-action");
  save_grid_organism_button.SetAttr("role", "menuitem");
  UI::Button remove_grid_organism_button{
    [this](){
      HideGridCellMenu();
      RemoveGridCellOrganism(grid_context_cell_id);
    },
    PresentationText("Remove Organism").str(),
    "remove_grid_organism_button"
  };
  remove_grid_organism_button.AddAttr(
    "class", "grid-cell-menu-action grid-cell-menu-remove"
  );
  remove_grid_organism_button.SetAttr("role", "menuitem");
  grid_cell_menu << save_grid_organism_button;
  grid_cell_menu << remove_grid_organism_button;
  population_surface << grid_cell_menu;
  if (!run_started && placed_organisms.size()) {
    const std::string staged_count = std::to_string(placed_organisms.size());
    const avida::web::localization::NamedArgument count_argument{"count", staged_count};
    const auto staged_text = avida::web::localization::Format(
      placed_organisms.size() == 1 ? "shell.staged_organisms.one" : "shell.staged_organisms.other",
      active_locale, {&count_argument, 1}
    );
    population_surface << emp::MakeString(
      "<div class='staged-organism-badge'>",
      staged_text ? emp::MakeWebSafe(staged_text->text) : emp::MakeWebSafe(staged_count),
      "</div>"
    );
  }
  canvas_frame << population_surface;

  UI::Div transport{"transport"};
  transport.AddAttr("class", "transport");
  UI::Div transport_buttons{"transport_buttons"};
  transport_buttons.AddAttr("class", "transport-buttons");

  restart_button = UI::Button(
    [this](){ RestartPopulation(); },
    "<span class='restart-icon' aria-hidden='true'></span>",
    "restart_button"
  );
  step_button = UI::Button(
    [this](){ StepPopulation(); },
    "<span class='step-icon' aria-hidden='true'></span>",
    "step_button"
  );
  play_button = UI::Button(
    [this](){ ToggleRunMode(RunMode::PLAY); }, "&#x25B6;", "play_button"
  );
  pause_button = UI::Button(
    [this](){ SetRunMode(RunMode::PAUSED); }, "&#x275A;&#x275A;", "pause_button"
  );
  fast_forward_button = UI::Button(
    [this](){ ToggleRunMode(RunMode::FAST_FORWARD); },
    "&#x25B6;&#x25B6;",
    "fast_forward_button"
  );

  restart_button.AddAttr("class", "transport-button icon-button restart-button");
  step_button.AddAttr("class", "transport-button icon-button");
  play_button.AddAttr("class", "transport-button icon-button");
  pause_button.AddAttr("class", "transport-button icon-button is-active");
  fast_forward_button.AddAttr("class", "transport-button icon-button");
  restart_button.SetAttr("aria-label", PresentationText("Start a new run"));
  step_button.SetAttr("aria-label", PresentationText("Advance one population update"));
  play_button.SetAttr("aria-label", PresentationText("Play at up to ten updates per second"));
  pause_button.SetAttr("aria-label", PresentationText("Pause evolution"));
  fast_forward_button.SetAttr("aria-label", PresentationText("Run as fast as possible"));
  restart_button.SetTitle(PresentationText("Start a new run; keep recorded results on the run shelf."));
  step_button.SetTitle(PresentationText("Step (Space)"));
  play_button.SetTitle(PresentationText("Play"));
  pause_button.SetTitle(PresentationText("Pause"));
  fast_forward_button.SetTitle(PresentationText("Fast-forward (>)"));
  color_selector.AddAttr("class", "color-selector");
  continuous_palette_selector.AddAttr("class", "color-selector palette-selector");
  transport_buttons << restart_button;
  transport_buttons << step_button;
  transport_buttons << play_button;
  transport_buttons << fast_forward_button;
  transport_buttons << pause_button;
  transport_buttons << color_selector;
  transport_buttons << continuous_palette_selector;
  transport << transport_buttons;
  population_card << canvas_frame;
  population_card << transport;

  run_inspector = UI::Div{"run_inspector"};
  run_inspector.AddAttr("class", "run-inspector");
  run_inspector << emp::MakeString(
    "<section class='lesson-intro'><h2>",
    emp::MakeWebSafe(Localized(avida::web::localization::key::LESSON_TITLE)),
    "</h2><p>",
    emp::MakeWebSafe(Localized(avida::web::localization::key::LESSON_QUESTION)),
    "</p><p class='lesson-note'>",
    emp::MakeWebSafe(PresentationText(
      "Run the same 10 × 10 world with three fixed seeds. Compare observations; a single run is not a general result."
    )),
    "</p><p class='lesson-note'>",
    emp::MakeWebSafe(PresentationText(
      "The bundled ancestor, CPU and instruction set, grid, and ten logic-task rewards stay fixed. This is selection-active evolution, not neutral evolution."
    )),
    "</p><p class='lesson-note'>",
    emp::MakeWebSafe(PresentationText(
      "Sequence richness counts distinct ordered sequences present now. It is not a mutation count or every sequence ever seen. More sequences do not establish greater fitness or adaptation. Treat each run as an observation, not a universal outcome."
    )),
    "</p></section>"
  );
  UI::Div experiment_controls{"experiment-controls"};
  experiment_controls.AddAttr("class", "experiment-controls");
  experiment_controls << emp::MakeString("<h2>", emp::MakeWebSafe(PresentationText("Experiment setup")), "</h2>");
  UI::Div experiment_field{"experiment-field"};
  experiment_field.AddAttr("class", "education-field education-preset-field");
  experiment_field << emp::MakeString("<label for='experiment-preset'>", emp::MakeWebSafe(PresentationText("Experiment preset")), "</label>");
  experiment_preset_selector.SetOption(
    PresentationText("Mutation variation · every update").str(),
    [this](){ SetEducationPreset("mutation-variation"); }
  );
  experiment_preset_selector.SetOption(
    PresentationText("Quick check · every 10 updates").str(),
    [this](){ SetEducationPreset("mutation-variation-quick"); }
  );
  experiment_preset_selector.SelectID(education_uses_quick_preset ? 1 : 0);
  experiment_preset_selector.SetAttr("aria-label", PresentationText("Experiment preset"));
  experiment_preset_selector.AddAttr("class", "education-select");
  experiment_field << experiment_preset_selector;
  experiment_controls << experiment_field;
  UI::Div treatment_field{"treatment-field"};
  treatment_field.AddAttr("class", "education-field");
  treatment_field << emp::MakeString(
    "<label for='run-preset'>", emp::MakeWebSafe(PresentationText("Mutation treatment")), "</label>"
  );
  run_preset_selector.SetOption(
    PresentationText("0% mutation").str(), [this](){ SetEducationTreatment("no-mutation"); }
  );
  run_preset_selector.SetOption(
    PresentationText("1% mutation").str(), [this](){ SetEducationTreatment("one-percent"); }
  );
  run_preset_selector.SelectID(
    Avida().GetSettings().Get<double>("mutations.substitution_prob") >= 0.005 ? 1 : 0
  );
  run_preset_selector.AddAttr("class", "education-select");
  run_preset_selector.SetAttr("aria-label", PresentationText("Mutation treatment"));
  treatment_field << run_preset_selector;
  experiment_controls << treatment_field;
  UI::Div mutation_field{"mutation-field"};
  mutation_field.AddAttr("class", "education-field");
  mutation_field << emp::MakeString(
    "<label for='run-mutation-percent'>",
    emp::MakeWebSafe(PresentationText("Per-site mutation setting (%)")), "</label>"
  );
  run_mutation_input = UI::Input{[](std::string){}, "number", "", "run-mutation-percent"};
  run_mutation_input.Value(emp::MakeString(
    Avida().GetSettings().Get<double>("mutations.substitution_prob") * 100.0
  ));
  run_mutation_input.Disabled(true);
  run_mutation_input.AddAttr("class", "education-input");
  mutation_field << run_mutation_input;
  experiment_controls << mutation_field;
  UI::Div seed_field{"seed-field"};
  seed_field.AddAttr("class", "education-field");
  seed_field << emp::MakeString(
    "<label for='run-seed'>", emp::MakeWebSafe(PresentationText("Random seed")), "</label>"
  );
  run_seed_input = UI::Input{
    [this](std::string value){ SetEducationSeed(value); }, "number", "", "run-seed"
  };
  run_seed_input.Min("42");
  run_seed_input.Max("44");
  run_seed_input.Step("1");
  run_seed_input.Value(emp::MakeString(education_seed));
  run_seed_input.AddAttr("class", "education-input");
  seed_field << run_seed_input;
  experiment_controls << seed_field;
  UI::Div target_field{"target-field"};
  target_field.AddAttr("class", "education-field");
  target_field << emp::MakeString(
    "<label for='run-target'>", emp::MakeWebSafe(PresentationText("Stop at update")), "</label>"
  );
  run_target_input = UI::Input{
    [this](std::string value){
      if (value.empty() || SimulationWorkerBusy() || run_mode != RunMode::PAUSED) return;
      const size_t parsed = emp::String{value}.As<size_t>();
      if (parsed > Avida().GetUpdate() && parsed <= 10000) {
        education_target_update = parsed;
        // Input::DoChange tracks the user value separately from the rendered
        // value. Keep both in sync so disabling/rebuilding the control cannot
        // restore its previous preset target.
        run_target_input.Value(value);
      }
      RefreshEducationStatus();
      UpdateControls();
    }, "number", "", "run-target"
  };
  run_target_input.Min("1");
  run_target_input.Max("10000");
  run_target_input.Step("1");
  run_target_input.Value(emp::MakeString(education_target_update));
  run_target_input.AddAttr("class", "education-input");
  target_field << run_target_input;
  experiment_controls << target_field;
  UI::Button run_to_target_button{
    [this](){ RunEducationToTarget(); },
    Localized("experiment.run_to_target").str(), "run_button"
  };
  run_to_target_button.AddAttr("class", "education-run-button");
  experiment_controls << run_to_target_button;
  education_status_text.Clear();
  education_status_text.AddAttr("role", "status");
  education_status_text.AddAttr("aria-live", "polite");
  education_status_text.AddAttr("class", "education-status");
  education_status_text << UI::Live([this](){
    const bool waiting_for_boundary =
      run_mode == RunMode::PAUSED && SimulationWorkerBusy();
    const size_t update = published_education_update.load(std::memory_order_acquire);
    const bool reached_target = run_mode == RunMode::PAUSED
      && update >= education_target_update;
    const std::string update_value = std::to_string(update);
    const std::string target_value = std::to_string(education_target_update);
    const std::array<avida::web::localization::NamedArgument, 2> args{{
      {"update", update_value}, {"target", target_value}
    }};
    const avida::web::localization::NamedArgument update_arg{"update", update_value};
    const std::string_view status_id = waiting_for_boundary ? "status.pausing"
      : reached_target ? "status.finished"
      : run_mode == RunMode::PAUSED ? "status.paused" : "status.update";
    return emp::MakeString(
      status_id == "status.update" ? Localized(status_id, args)
        : Localized(status_id, {&update_arg, 1}),
      education_archive_status.empty() ? "" : " · ",
      PresentationText(education_archive_status)
    );
  });
  experiment_controls << education_status_text;
  run_inspector << experiment_controls;
  run_inspector << emp::MakeString(
    "<h2>", emp::MakeWebSafe(PresentationText("Current population")), "</h2>"
  );
  UI::Div readouts{"readouts"};
  readouts.AddAttr("class", "readouts");
  const auto & statistics = population_view_options.GetStatistics();
  statistic_texts.reserve(statistics.size());
  for (size_t statistic_id = 0; statistic_id < statistics.size(); ++statistic_id) {
    const auto & statistic = statistics[statistic_id];
    UI::Div readout{emp::MakeString("statistic_", statistic_id)};
    readout.AddAttr("class", "readout");
    readout.SetAttr(
      "data-metric", statistic.id == "organisms" ? "population_count" : statistic.id
    );
    readout.SetTitle(PresentationText(statistic.description));
    const emp::String display_label = statistic.id == "organisms"
      ? Localized(avida::web::localization::key::METRIC_ORGANISMS)
      : PresentationText(statistic.label);
    readout << emp::MakeString("<span>", emp::MakeWebSafe(display_label), "</span>");

    UI::Text value_text{emp::MakeString("statistic_value_", statistic_id)};
    value_text << UI::Live([this, statistic_id](){
      return GetStatisticValue(statistic_id);
    });
    readout << value_text;
    statistic_texts.push_back(value_text);
    readouts << readout;
  }
  run_inspector << readouts;
  population_color_legend = UI::Text{"population_color_legend"};
  population_color_legend << UI::Live([this](){
    return BuildPopulationColorLegendHTML();
  });
  run_inspector << population_color_legend;
  education_results_text.Clear();
  education_results_text << UI::Live([this](){ return BuildEducationResultsHTML(); });
  run_inspector << education_results_text;
  run_history_text.Clear();
  run_history_text << UI::Live([this](){ return BuildRunHistoryHTML(); });
  run_inspector << run_history_text;

  UI::Div learning_record{"learning-record"};
  learning_record.AddAttr("class", "learning-record");
  learning_record << emp::MakeString("<h2>", emp::MakeWebSafe(PresentationText("Learning record")), "</h2><p>", emp::MakeWebSafe(PresentationText("Write a prediction before running, then record what you observed. These notes travel with the saved experiment.")), "</p>");
  prediction_notes = UI::TextArea{
    [this](const std::string & value){ prediction_note = value; }, "prediction-notes"
  };
  prediction_notes.SetText(prediction_note);
  prediction_notes.AddAttr("aria-label", PresentationText("Prediction before running"));
  prediction_notes.AddAttr("placeholder", PresentationText("What do you expect to happen?"));
  observation_notes = UI::TextArea{
    [this](const std::string & value){ observation_note = value; }, "observation-notes"
  };
  observation_notes.SetText(observation_note);
  observation_notes.AddAttr("aria-label", PresentationText("Observation after running"));
  observation_notes.AddAttr("placeholder", PresentationText("What did the recorded runs show?"));
  learning_record << emp::MakeString("<label for='prediction-notes'>", emp::MakeWebSafe(PresentationText("Prediction")), "</label>") << prediction_notes;
  learning_record << emp::MakeString("<label for='observation-notes'>", emp::MakeWebSafe(PresentationText("Observations")), "</label>") << observation_notes;
  UI::Div archive_controls{"education-archive-controls"};
  archive_controls.AddAttr("class", "education-archive-controls");
  UI::Button save_education{
    [this](){ SaveEducationExperiment(); }, PresentationText("Save experiment JSON").str(), "education-save"
  };
  UI::Button export_education{
    [this](){ ExportEducationData(); }, PresentationText("Export completed data CSV").str(), "education-export-csv"
  };
  UI::FileInput open_education{
    [this](const std::string & value){ ImportEducationExperiment(value); }, "education-open-file"
  };
  open_education.AddAttr("accept", ".json,application/json");
  open_education.AddAttr("aria-label", PresentationText("Open saved education experiment JSON"));
  archive_controls << save_education << export_education;
  archive_controls << emp::MakeString("<label for='education-open-file'>", emp::MakeWebSafe(PresentationText("Open experiment JSON")), "</label>") << open_education;
  learning_record << archive_controls;
  UI::Text archive_status{"education-archive-status"};
  archive_status.AddAttr("class", "education-status");
  archive_status.AddAttr("role", "status");
  archive_status.AddAttr("aria-live", "polite");
  archive_status << UI::Live([this](){ return emp::MakeWebSafe(education_archive_status); });
  learning_record << archive_status;
  run_inspector << learning_record;

  org_stats_inspector = UI::Div{"org_stats_inspector"};
  org_stats_inspector.AddAttr("class", "org-stats-inspector");
  org_stats_inspector.SetAttr("role", "region");
  org_stats_inspector.SetAttr("aria-label", PresentationText("Selected cell"));
  org_stats_inspector << emp::MakeString(
    "<h2>", emp::MakeWebSafe(PresentationText("Selected cell")), "</h2>"
  );
  org_stats_content = UI::Text{"org_stats_content"};
  org_stats_content << UI::Live([this](){ return BuildOrganismStatsHTML(); });
  org_stats_inspector << org_stats_content;

  BuildFreezerPanel();
  legacy_sidebar << freezer_inspector;
  BuildConfigurationPanel();

  workspace << population_card;
  UI::Div side_panel{"side_panel"};
  side_panel.AddAttr("class", "side-panel");
  side_panel << run_inspector;
  side_panel << org_stats_inspector;
  side_panel << configuration_inspector;
  workspace << side_panel;
  main_region << legacy_sidebar;
  main_region << workspace;
  app << main_region;
  document << app;
}

void AvidaWebApp::BuildCompareModeWorkspace(UI::Div & app) {
  UI::Div workspace{"compare-workspace"};
  workspace.AddAttr("class", "compare-workspace");
  workspace << emp::MakeString(
    "<section class='lesson-intro'><h1>", emp::MakeWebSafe(PresentationText("Compare completed runs")),
    "</h1><p>", emp::MakeWebSafe(PresentationText("Each line is an independent stochastic trajectory. The three seeds are reused across treatments as starting values; they do not guarantee paired random histories.")),
    "</p><p>", emp::MakeWebSafe(PresentationText("Compare population size and exact sequence richness. An endpoint table reports the final sample of every completed run.")),
    "</p></section>"
  );
  UI::Button back{
    [this](){ SetApplicationMode(ApplicationMode::POPULATION); },
    PresentationText("Back to population and run setup").str(), "back-population-mode"
  };
  back.AddAttr("class", "education-run-button");
  workspace << back;
  UI::Text comparison{"comparison-results"};
  comparison << UI::Live([this](){ return BuildComparisonHTML(); });
  workspace << comparison;
  app << workspace;
}

void AvidaWebApp::RebuildInterface() {
  const SidePanel restore_side_panel = active_side_panel;
  document.Freeze();
  document.ClearChildren();
  configuration_inputs.clear();
  configuration_selectors.clear();
  statistic_texts.clear();
  SetupColorSelector();
  BuildInterface();
  document.Activate();
  UpdateEducationPlots();
  if (active_application_mode == ApplicationMode::POPULATION) {
    SetSidePanel(restore_side_panel);
    UpdateControls();
    DrawPopulation();
    RefreshReadouts();
  } else {
    UpdateOrganismModeControls();
    FocusTrackedOrganismHead();
  }
  EM_ASM({
    const language = document.getElementById("language-selector");
    if (language) {
      if (language.options[0]) language.options[0].value = "en";
      if (language.options[1]) language.options[1].value = "es";
      if (language.options[2]) language.options[2].value = "qps-ploc";
      if (language.options[3]) language.options[3].value = "qps-plocm";
    }
    const preset = document.getElementById("run-preset");
    if (preset) {
      if (preset.options[0]) preset.options[0].value = "no-mutation";
      if (preset.options[1]) preset.options[1].value = "one-percent";
    }
    const experiment = document.getElementById("experiment-preset");
    if (experiment) {
      if (experiment.options[0]) experiment.options[0].value = "mutation-variation";
      if (experiment.options[1]) experiment.options[1].value = "mutation-variation-quick";
    }
    document.documentElement.lang = UTF8ToString($0);
    document.documentElement.dir = $1 ? "rtl" : "ltr";
  }, active_locale == avida::web::localization::Locale::ENGLISH ? "en"
       : active_locale == avida::web::localization::Locale::SPANISH ? "es"
         : active_locale == avida::web::localization::Locale::PSEUDO_EXPANDED ? "qps-ploc" : "qps-plocm",
     active_locale == avida::web::localization::Locale::PSEUDO_RTL);
}
