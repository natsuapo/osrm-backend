#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "storage/shared_barriers.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/engine_config.hpp"
#include "engine/plugins/match.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/table.hpp"
#include "engine/plugins/tile.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
#include "engine/status.hpp"
#include "util/json_container.hpp"

#include <memory>
#include <string>

namespace osrm
{
namespace engine
{

class Engine final
{
  public:
    explicit Engine(const EngineConfig &config);

    // Moveable
    Engine(Engine &&) noexcept = default;
    Engine &operator=(Engine &&) noexcept = default;

    // Not copyable
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    Status Route(const api::RouteParameters &parameters, util::json::Object &result) const;
    Status Table(const api::TableParameters &parameters, util::json::Object &result) const;
    Status Nearest(const api::NearestParameters &parameters, util::json::Object &result) const;
    Status Trip(const api::TripParameters &parameters, util::json::Object &result) const;
    Status Match(const api::MatchParameters &parameters, util::json::Object &result) const;
    Status Tile(const api::TileParameters &parameters, std::string &result) const;

  private:
    std::unique_ptr<storage::SharedBarriers> lock;

    const plugins::ViaRoutePlugin route_plugin;
    const plugins::TablePlugin table_plugin;
    const plugins::NearestPlugin nearest_plugin;
    const plugins::TripPlugin trip_plugin;
    const plugins::MatchPlugin match_plugin;
    const plugins::TilePlugin tile_plugin;

    std::shared_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
