#include "probfd/heuristics/pdbs/maxprob_pattern_database.h"

#include "probfd/engines/interval_iteration.h"

#include "probfd/utils/graph_visualization.h"

#include "pdbs/pattern_database.h"

#include "utils/collections.h"

#include "lp/lp_solver.h"

#include <deque>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace probfd {
namespace heuristics {
namespace pdbs {

MaxProbPatternDatabase::MaxProbPatternDatabase(
    const ProbabilisticTaskProxy& task_proxy,
    const Pattern& pattern,
    TaskCostFunction& task_cost_function,
    const State& initial_state,
    bool operator_pruning,
    const StateRankEvaluator& heuristic)
    : ProbabilisticPatternDatabase(task_proxy, pattern)
{
    ProjectionStateSpace state_space(
        task_proxy,
        ranking_function_,
        operator_pruning);
    ProjectionCostFunction cost_function(
        task_proxy,
        ranking_function_,
        &task_cost_function);
    compute_value_table(
        state_space,
        cost_function,
        ranking_function_.rank(initial_state),
        heuristic);
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    ProjectionStateSpace& state_space,
    StateRankingFunction ranking_function,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const StateRankEvaluator& heuristic)
    : ProbabilisticPatternDatabase(std::move(ranking_function))
{
    compute_value_table(state_space, cost_function, initial_state, heuristic);
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    const ProbabilisticTaskProxy& task_proxy,
    const ::pdbs::PatternDatabase& pdb,
    TaskCostFunction& task_cost_function,
    const State& initial_state,
    bool operator_pruning)
    : MaxProbPatternDatabase(
          task_proxy,
          pdb.get_pattern(),
          task_cost_function,
          initial_state,
          operator_pruning,
          DeadendPDBEvaluator(pdb))
{
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    ProjectionStateSpace& state_space,
    StateRankingFunction ranking_function,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const ::pdbs::PatternDatabase& pdb)
    : MaxProbPatternDatabase(
          state_space,
          std::move(ranking_function),
          cost_function,
          initial_state,
          DeadendPDBEvaluator(pdb))
{
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    const ProbabilisticTaskProxy& task_proxy,
    const MaxProbPatternDatabase& pdb,
    int add_var,
    TaskCostFunction& task_cost_function,
    const State& initial_state,
    bool operator_pruning)
    : ProbabilisticPatternDatabase(
          task_proxy,
          utils::insert(pdb.get_pattern(), add_var))
{
    ProjectionStateSpace state_space(
        task_proxy,
        ranking_function_,
        operator_pruning);
    ProjectionCostFunction cost_function(
        task_proxy,
        ranking_function_,
        &task_cost_function);
    compute_value_table(
        state_space,
        cost_function,
        ranking_function_.rank(initial_state),
        IncrementalPPDBEvaluator(pdb, &ranking_function_, add_var));
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    ProjectionStateSpace& state_space,
    StateRankingFunction ranking_function,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const MaxProbPatternDatabase& pdb,
    int add_var)
    : ProbabilisticPatternDatabase(std::move(ranking_function))
{
    compute_value_table(
        state_space,
        cost_function,
        initial_state,
        IncrementalPPDBEvaluator(pdb, &ranking_function_, add_var));
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    const ProbabilisticTaskProxy& task_proxy,
    const MaxProbPatternDatabase& left,
    const MaxProbPatternDatabase& right,
    TaskCostFunction& task_cost_function,
    const State& initial_state,
    bool operator_pruning)
    : ProbabilisticPatternDatabase(
          task_proxy,
          utils::merge_sorted(left.get_pattern(), right.get_pattern()))
{
    ProjectionStateSpace state_space(
        task_proxy,
        ranking_function_,
        operator_pruning);
    ProjectionCostFunction cost_function(
        task_proxy,
        ranking_function_,
        &task_cost_function);
    compute_value_table(
        state_space,
        cost_function,
        ranking_function_.rank(initial_state),
        MergeEvaluator(ranking_function_, left, right));
}

MaxProbPatternDatabase::MaxProbPatternDatabase(
    ProjectionStateSpace& state_space,
    StateRankingFunction ranking_function,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const MaxProbPatternDatabase& left,
    const MaxProbPatternDatabase& right)
    : ProbabilisticPatternDatabase(std::move(ranking_function))
{
    compute_value_table(
        state_space,
        cost_function,
        initial_state,
        MergeEvaluator(ranking_function_, left, right));
}

void MaxProbPatternDatabase::compute_value_table(
    ProjectionStateSpace& state_space,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const StateRankEvaluator& heuristic)
{
    using namespace engines::interval_iteration;

    IntervalIteration<StateRank, const AbstractOperator*>
        vi(&state_space, &cost_function, &heuristic, true, true);

    std::vector<StateID> proper_states;

    std::vector<Interval> interval_value_table(
        ranking_function_.num_states(),
        Interval(INFINITE_VALUE));

    vi.solve(initial_state, interval_value_table, dead_ends_, proper_states);

    // We only need the upper bounds
    for (std::size_t i = 0; i != interval_value_table.size(); ++i) {
        value_table[i] = interval_value_table[i].upper;
    }

#if !defined(NDEBUG)
    std::cout << "(II) Pattern [";
    for (unsigned i = 0; i < ranking_function_.get_pattern().size(); ++i) {
        std::cout << (i > 0 ? ", " : "") << ranking_function_.get_pattern()[i];
    }

    std::cout << "]: value=" << interval_value_table[initial_state.id]
              << std::endl;

#if defined(USE_LP)
    verify(state_space, cost_function, initial_state);
#endif
#endif
}

EvaluationResult MaxProbPatternDatabase::evaluate(const State& s) const
{
    return evaluate(get_abstract_state(s));
}

EvaluationResult MaxProbPatternDatabase::evaluate(StateRank s) const
{
    if (is_dead_end(s)) {
        return {true, 1_vt};
    }

    return {false, this->lookup(s)};
}

std::unique_ptr<AbstractPolicy>
MaxProbPatternDatabase::get_optimal_abstract_policy(
    ProjectionStateSpace& state_space,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const std::shared_ptr<utils::RandomNumberGenerator>& rng,
    bool wildcard) const
{
    return ProbabilisticPatternDatabase::get_optimal_abstract_policy(
        state_space,
        cost_function,
        initial_state,
        rng,
        wildcard,
        false);
}

std::unique_ptr<AbstractPolicy>
MaxProbPatternDatabase::get_optimal_abstract_policy_no_traps(
    ProjectionStateSpace& state_space,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const std::shared_ptr<utils::RandomNumberGenerator>& rng,
    bool wildcard) const
{
    return ProbabilisticPatternDatabase::get_optimal_abstract_policy_no_traps(
        state_space,
        cost_function,
        initial_state,
        rng,
        wildcard,
        false);
}

void MaxProbPatternDatabase::dump_graphviz(
    ProjectionStateSpace& state_space,
    ProjectionCostFunction& cost_function,
    StateRank initial_state,
    const std::string& path,
    bool transition_labels)
{
    auto s2str = [this](StateRank x) {
        std::ostringstream out;
        out.precision(3);
        out << x.id << "\\n";

        if (is_dead_end(x)) {
            out << "h = 1 (dead)";
        } else {
            out << "h = " << value_table[x.id];
        }

        return out.str();
    };

    ProbabilisticPatternDatabase::dump_graphviz(
        state_space,
        cost_function,
        initial_state,
        path,
        s2str,
        transition_labels);
}

#if !defined(NDEBUG) && defined(USE_LP)
void MaxProbPatternDatabase::verify(
    ProjectionStateSpace& state_space,
    ProjectionCostFunction& cost_function,
    StateRank initial_state)
{
    lp::LPSolverType type;

#ifdef COIN_HAS_CLP
    type = lp::LPSolverType::CLP;
#elif defined(COIN_HAS_CPX)
    type = lp::LPSolverType::CPLEX;
#elif defined(COIN_HAS_GRB)
    type = lp::LPSolverType::GUROBI;
#elif defined(COIN_HAS_SPX)
    type = lp::LPSolverType::SOPLEX;
#else
    std::cerr << "Warning: Could not verify PDB value table since no LP solver"
                 "is available !"
              << std::endl;
    return;
#endif

    lp::LPSolver solver(type);
    const double inf = solver.get_infinity();

    named_vector::NamedVector<lp::LPVariable> variables;

    for (StateRank s = StateRank(0);
         s.id != static_cast<int>(ranking_function_.num_states());
         ++s.id) {
        variables.emplace_back(0_vt, 1_vt, 1_vt);
    }

    named_vector::NamedVector<lp::LPConstraint> constraints;

    std::deque<StateRank> queue({initial_state});
    std::set<StateRank> seen({initial_state});

    while (!queue.empty()) {
        StateRank s = queue.front();
        queue.pop_front();

        if (cost_function.is_goal(s)) {
            auto& g = constraints.emplace_back(0_vt, 0_vt);
            g.insert(s.id, 1_vt);
        }

        // Generate operators...
        std::vector<const AbstractOperator*> aops;
        state_space.generate_applicable_actions(s.id, aops);

        // Select a greedy operators and add its successors
        for (const AbstractOperator* op : aops) {
            auto& constr = constraints.emplace_back(-inf, 0_vt);

            std::unordered_map<StateRank, value_t> successor_dist;

            for (const auto& [eff, prob] : op->outcomes) {
                successor_dist[s + eff] -= prob;
            }

            if (successor_dist.size() == 1 &&
                successor_dist.begin()->first == s) {
                continue;
            }

            successor_dist[s] += 1_vt;

            for (const auto& [succ, prob] : successor_dist) {
                constr.insert(succ.id, prob);

                if (seen.insert(succ).second) {
                    queue.push_back(succ);
                }
            }
        }
    }

    solver.load_problem(lp::LinearProgram(
        lp::LPObjectiveSense::MAXIMIZE,
        std::move(variables),
        std::move(constraints),
        inf));

    solver.solve();

    assert(solver.has_optimal_solution());

    std::vector<double> solution = solver.extract_solution();

    for (StateRank s(0); s.id != static_cast<int>(value_table.size()); ++s.id) {
        if (seen.contains(s)) {
            assert(is_approx_equal(solution[s.id], value_table[s.id], 0.001));
        } else {
            assert(value_table[s.id] == INFINITE_VALUE);
        }
    }
}
#endif

} // namespace pdbs
} // namespace heuristics
} // namespace probfd
