create_fast_downward_library(
    NAME mdp
    HELP "Core source files for supporting MDPs"
    SOURCES
        # Main
        probfd/command_line

        # Evaluators
        probfd/evaluator

        # Tasks
        probfd/probabilistic_task

        probfd/task_utils/task_properties

        # Basic types
        probfd/evaluation_result
        probfd/interval
        probfd/value_type
        
        # MDP interfaces
        probfd/state_evaluator
        probfd/cost_function
        probfd/caching_task_state_space
        probfd/task_state_space
        probfd/progress_report
        probfd/quotient_system

        # Algorithms
        probfd/algorithms/utils

        # Cost Functions
        probfd/maxprob_cost_function
        probfd/ssp_cost_function

        # Task Cost Function Factories
        probfd/task_cost_function_factory

        # Task Evaluator Factories
        probfd/task_evaluator_factory

        # Constant evaluator (default)
        probfd/heuristics/constant_evaluator

        # Utility
        probfd/utils/guards.cc

        probfd/solver_interface
        
        probfd/solvers/mdp_solver
    DEPENDS probabilistic_successor_generator
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME probabilistic_successor_generator
    HELP "Probabilistic Successor generator"
    SOURCES
        probfd/task_utils/probabilistic_successor_generator
        probfd/task_utils/probabilistic_successor_generator_factory
        probfd/task_utils/probabilistic_successor_generator_internals
    DEPENDS task_properties
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME core_probabilistic_tasks
    HELP "Core probabilistic task transformations"
    SOURCES
        probfd/tasks/cost_adapted_task
        probfd/tasks/delegating_task
        probfd/tasks/root_task
        probfd/tasks/all_outcomes_determinization
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME extra_probabilistic_tasks
    HELP "Additional probabilistic task transformations"
    SOURCES
        probfd/tasks/domain_abstracted_task
        probfd/tasks/domain_abstracted_task_factory
        probfd/tasks/modified_goals_task
        probfd/tasks/modified_operator_costs_task
    CORE_LIBRARY
)

create_fast_downward_library(
    NAME bisimulation_core
    HELP "bisimulation_core"
    SOURCES
        probfd/bisimulation/bisimilar_state_space
        probfd/bisimulation/evaluators
    DEPENDS mdp
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME acyclic_value_iteration_solver
    HELP "acyclic_vi"
    SOURCES
        probfd/solvers/acyclic_vi
    DEPENDS mdp
)

create_fast_downward_library(
    NAME topological_value_iteration_solver
    HELP "topological_vi"
    SOURCES
        probfd/solvers/topological_vi
    DEPENDS mdp
)

create_fast_downward_library(
    NAME interval_iteration_solver
    HELP "interval_iteration"
    SOURCES
        probfd/solvers/interval_iteration
    DEPENDS mdp
)

create_fast_downward_library(
    NAME idual_solver
    HELP "idual solver"
    SOURCES
        probfd/solvers/idual
    DEPENDS mdp lp_solver
)

create_fast_downward_library(
    NAME i2dual_solver
    HELP "i2dual solvers"
    SOURCES
        probfd/algorithms/i2dual
        probfd/solvers/i2dual
    DEPENDS mdp lp_solver occupation_measure_heuristics
)

create_fast_downward_library(
    NAME bisimulation_based_solver
    HELP "bisimulation_vi"
    SOURCES
        probfd/solvers/bisimulation_vi
    DEPENDS bisimulation_core
)

create_fast_downward_library(
    NAME mdp_heuristic_search_base
    HELP "mdp heuristic search core"
    SOURCES
        # Open Lists
        probfd/open_lists/subcategory

        # Transition Samplers
        probfd/successor_sampler
        probfd/successor_samplers/subcategory

        # Policy Tiebreakers
        probfd/policy_pickers/operator_id_tiebreaker
        probfd/policy_pickers/subcategory

        # Successor Sorters
        probfd/transition_sorters/vdiff_sorter
        probfd/transition_sorters/vdiff_sorter_factory
        probfd/transition_sorters/subcategory

        # Base heuristic search solver
        probfd/solvers/mdp_heuristic_search
    DEPENDENCY_ONLY
    DEPENDS mdp
)

create_fast_downward_library(
    NAME ao_search
    HELP "aostar implementations"
    SOURCES
        probfd/solvers/aostar
        probfd/solvers/exhaustive_ao
    DEPENDS mdp_heuristic_search_base bisimulation_core
)

create_fast_downward_library(
    NAME exhaustive_dfhs
    HELP "exhaustive depth-first heuristic search"
    SOURCES
        probfd/solvers/exhaustive_dfs
    DEPENDS mdp_heuristic_search_base
)

create_fast_downward_library(
    NAME lrtdp_solver
    HELP "lrtdp"
    SOURCES
        probfd/solvers/lrtdp
    DEPENDS mdp_heuristic_search_base bisimulation_core
)

create_fast_downward_library(
    NAME trap_aware_lrtdp_solver
    HELP "Trap-Aware LRTDP (TALRTDP)"
    SOURCES
        probfd/solvers/talrtdp
    DEPENDS mdp_heuristic_search_base
)

create_fast_downward_library(
    NAME trap_aware_dfhs_solver
    HELP "Trap-Aware DFHS (TADFHS)"
    SOURCES
        probfd/solvers/tadfhs
    DEPENDS mdp_heuristic_search_base
)

create_fast_downward_library(
    NAME trap_aware_topological_value_iteration_solver
    HELP "Trap-Aware Topological Value Iteration"
    SOURCES
        probfd/solvers/ta_topological_vi
    DEPENDS mdp
)

create_fast_downward_library(
    NAME dfhs_solver
    HELP "depth-first heuristic search"
    SOURCES
        probfd/solvers/hdfs
    DEPENDS mdp_heuristic_search_base bisimulation_core
)

create_fast_downward_library(
    NAME task_dependent_heuristic
    HELP "Heuristics depending on the input task"
    SOURCES
        probfd/heuristics/task_dependent_heuristic
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME deadend_pruning_heuristic
    HELP "Dead-end pruning heuristic"
    SOURCES
        probfd/heuristics/dead_end_pruning
    DEPENDS successor_generator task_dependent_heuristic
)

create_fast_downward_library(
    NAME determinization_heuristic
    HELP "All-outcomes determinization heuristic"
    SOURCES
        probfd/heuristics/determinization_cost
    DEPENDS successor_generator task_dependent_heuristic
)

create_fast_downward_library(
    NAME lp_based_heuristic
    HELP "LP-based heuristic"
    SOURCES
        probfd/heuristics/lp_heuristic
    DEPENDS mdp lp_solver task_dependent_heuristic
)

create_fast_downward_library(
    NAME occupation_measure_heuristics
    HELP "Occupation measure heuristics"
    SOURCES
        probfd/heuristics/occupation_measures/constraint_generator
        probfd/heuristics/occupation_measures/hpom_constraints
        probfd/heuristics/occupation_measures/hroc_constraints
        probfd/heuristics/occupation_measures/higher_order_hpom_constraints
        probfd/heuristics/occupation_measures/occupation_measure_heuristic
        probfd/heuristics/occupation_measures/pho_constraints
        probfd/heuristics/occupation_measures/subcategory
    DEPENDS mdp lp_based_heuristic
)

create_fast_downward_library(
    NAME probability_aware_pdbs
    HELP "Probability-aware PDBs base classes"
    SOURCES
        probfd/heuristics/pdbs/distances
        probfd/heuristics/pdbs/evaluators
        probfd/heuristics/pdbs/match_tree
        probfd/heuristics/pdbs/policy_extraction
        probfd/heuristics/pdbs/probability_aware_pattern_database
        probfd/heuristics/pdbs/projection_operator
        probfd/heuristics/pdbs/projection_state_space
        probfd/heuristics/pdbs/saturation
        probfd/heuristics/pdbs/state_ranking_function
    DEPENDS pdbs mdp task_dependent_heuristic
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME padbs_pattern_generators
    HELP "Base classes for pattern collection generation for PPDBs"
    SOURCES
        probfd/heuristics/pdbs/pattern_information
        probfd/heuristics/pdbs/pattern_generator
        probfd/heuristics/pdbs/pattern_collection_information
        probfd/heuristics/pdbs/pattern_collection_generator
        probfd/heuristics/pdbs/pattern_collection_generator_multiple

        probfd/heuristics/pdbs/subcollection_finder_factory
        probfd/heuristics/pdbs/subcollection_finder
        probfd/heuristics/pdbs/max_orthogonal_finder_factory
        probfd/heuristics/pdbs/max_orthogonal_finder
        probfd/heuristics/pdbs/trivial_finder_factory
        probfd/heuristics/pdbs/trivial_finder
        probfd/heuristics/pdbs/fully_additive_finder_factory
        probfd/heuristics/pdbs/fully_additive_finder
        probfd/heuristics/pdbs/trivial_finder
        probfd/heuristics/pdbs/subcollections

        probfd/heuristics/pdbs/utils
    DEPENDS probability_aware_pdbs max_cliques
    DEPENDENCY_ONLY
)

create_fast_downward_library(
    NAME papdbs_classical_generator
    HELP "Classical pattern collection generator adapter"
    SOURCES
        probfd/heuristics/pdbs/pattern_collection_generator_classical
    DEPENDS padbs_pattern_generators
)

create_fast_downward_library(
    NAME papdbs_hillclimbing_generator
    HELP "Hillclimbing pattern collection generator for PPDBs"
    SOURCES
        probfd/heuristics/pdbs/pattern_collection_generator_hillclimbing
    DEPENDS padbs_pattern_generators
)

create_fast_downward_library(
    NAME papdbs_cegar
    HELP "Disjoint CEGAR pattern collection generator for PPDBs"
    SOURCES
        probfd/heuristics/pdbs/cegar/cegar
        probfd/heuristics/pdbs/cegar/bfs_flaw_finder
        probfd/heuristics/pdbs/cegar/pucs_flaw_finder
        probfd/heuristics/pdbs/cegar/sampling_flaw_finder
        probfd/heuristics/pdbs/cegar/flaw_finding_strategy
    DEPENDS padbs_pattern_generators
)

create_fast_downward_library(
    NAME papdbs_disjoint_cegar_generator
    HELP "Disjoint CEGAR pattern collection generator for PPDBs"
    SOURCES
        probfd/heuristics/pdbs/pattern_collection_generator_disjoint_cegar
    DEPENDS papdbs_cegar
)

create_fast_downward_library(
    NAME papdbs_multiple_cegar_generator
    HELP "Multiple CEGAR pattern collection generator for PPDBs"
    SOURCES
        probfd/heuristics/pdbs/pattern_collection_generator_multiple_cegar
    DEPENDS papdbs_cegar
)

create_fast_downward_library(
    NAME probability_aware_pdb_heuristic
    HELP "Probability-aware PDB heuristic"
    SOURCES
        probfd/heuristics/pdbs/probability_aware_pdb_heuristic
    DEPENDS probability_aware_pdbs padbs_pattern_generators
)

create_fast_downward_library(
    NAME scp_pdb_heuristic
    HELP "Saturated Cost-Partitioning heuristic for probability-aware PDBs"
    SOURCES
        probfd/heuristics/cost_partitioning/scp_heuristic

    DEPENDS probability_aware_pdbs padbs_pattern_generators
)

create_fast_downward_library(
    NAME ucp_pdb_heuristic
    HELP "Uniform Cost-Partitioning heuristic for probability-aware PDBs"
    SOURCES
        probfd/heuristics/cost_partitioning/ucp_heuristic

    DEPENDS probability_aware_pdbs padbs_pattern_generators
)

create_fast_downward_library(
    NAME gzocp_pdb_heuristic
    HELP "Greedy Zero-One Cost-Partitioning heuristic for probability-aware PDBs"
    SOURCES
        probfd/heuristics/cost_partitioning/gzocp_heuristic

    DEPENDS probability_aware_pdbs padbs_pattern_generators
)

create_fast_downward_library(
    NAME pa_cartesian_abstractions
    HELP "The code for probability-aware cartesian abstractions"
    SOURCES
        probfd/heuristics/cartesian/abstract_state
        probfd/heuristics/cartesian/abstraction
        probfd/heuristics/cartesian/adaptive_flaw_generator
        probfd/heuristics/cartesian/additive_cartesian_heuristic
        probfd/heuristics/cartesian/astar_trace_generator
        probfd/heuristics/cartesian/cartesian_heuristic_function
        probfd/heuristics/cartesian/cegar
        probfd/heuristics/cartesian/cost_saturation
        probfd/heuristics/cartesian/complete_policy_flaw_finder
        probfd/heuristics/cartesian/distances
        probfd/heuristics/cartesian/evaluators
        probfd/heuristics/cartesian/flaw
        probfd/heuristics/cartesian/flaw_generator
        probfd/heuristics/cartesian/ilao_policy_generator
        probfd/heuristics/cartesian/policy_based_flaw_generator
        probfd/heuristics/cartesian/probabilistic_transition_system
        probfd/heuristics/cartesian/split_selector
        probfd/heuristics/cartesian/subtask_generators
        probfd/heuristics/cartesian/trace_based_flaw_generator
        probfd/heuristics/cartesian/utils
    DEPENDS cartesian_abstractions additive_heuristic extra_probabilistic_tasks
)
