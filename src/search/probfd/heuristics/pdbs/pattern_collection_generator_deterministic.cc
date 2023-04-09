#include "probfd/heuristics/pdbs/pattern_collection_generator_deterministic.h"

#include "probfd/heuristics/pdbs/subcollection_finder_factory.h"

#include "probfd/cost_model.h"

#include "options/options.h"

#include "pdbs/pattern_database.h"

#include "option_parser.h"
#include "plugin.h"

#include "probfd/tasks/all_outcomes_determinization.h"
#include "tasks/root_task.h"

namespace probfd {
namespace heuristics {
namespace pdbs {

PatternCollectionGeneratorDeterministic::
    PatternCollectionGeneratorDeterministic(
        std::shared_ptr<::pdbs::PatternCollectionGenerator> gen,
        std::shared_ptr<SubCollectionFinderFactory> finder_factory)
    : gen(gen)
    , finder_factory(finder_factory)
{
}

PatternCollectionGeneratorDeterministic::
    PatternCollectionGeneratorDeterministic(const options::Options& opts)
    : PatternCollectionGeneratorDeterministic(
          opts.get<std::shared_ptr<::pdbs::PatternCollectionGenerator>>(
              "generator"),
          opts.get<std::shared_ptr<SubCollectionFinderFactory>>(
              "subcollection_finder_factory"))
{
}

PatternCollectionInformation PatternCollectionGeneratorDeterministic::generate(
    const std::shared_ptr<ProbabilisticTask>& task)
{
    ProbabilisticTaskProxy task_proxy(*task);
    TaskCostFunction* task_cost_function = g_cost_model->get_cost_function();

    std::shared_ptr<tasks::AODDeterminizationTask> determinization(
        new tasks::AODDeterminizationTask(task.get()));

    std::shared_ptr<SubCollectionFinder> finder =
        finder_factory->create_subcollection_finder(task_proxy);
    return PatternCollectionInformation(
        ProbabilisticTaskProxy(*task),
        task_cost_function,
        gen->generate(determinization),
        finder);
}

std::shared_ptr<utils::Printable>
PatternCollectionGeneratorDeterministic::get_report() const
{
    return nullptr;
}

static std::shared_ptr<PatternCollectionGeneratorDeterministic>
_parse(OptionParser& parser)
{
    if (parser.dry_run()) {
        return nullptr;
    }

    parser.document_synopsis(
        "Pattern Generator Adapter for the All Outcomes Determinization",
        "Generates all the pattern collection according to the underlying "
        "generator for the deterministic problem.");

    parser.add_option<std::shared_ptr<::pdbs::PatternCollectionGenerator>>(
        "generator",
        "The underlying pattern generator for the deterministic problem.",
        "systematic()");

    parser.add_option<std::shared_ptr<SubCollectionFinderFactory>>(
        "subcollection_finder_factory",
        "The subcollection finder factory.",
        "finder_trivial_factory()");

    Options opts = parser.parse();
    if (parser.dry_run()) return nullptr;

    return std::make_shared<PatternCollectionGeneratorDeterministic>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("det_adapter", _parse);

} // namespace pdbs
} // namespace heuristics
} // namespace probfd