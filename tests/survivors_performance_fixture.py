"""Real visual reservation helpers with an injectable, initially disabled setting."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def helpers():
    source = (ROOT/'src/survivors.cpp').read_text()
    start = source.index('survivor_perf::Budget s_visualBudget;')
    end = source.index('void prepare_performance_visuals(', start)
    return '''
#include "survivors_performance_policy.hpp"
namespace dawnlight::survivor_perf {inline bool testEnabled=false;bool enabled(){return testEnabled;}}
namespace survivor_perf=dawnlight::survivor_perf;
''' + source[start:end]
