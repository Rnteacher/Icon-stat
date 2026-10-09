#pragma once

#include "Collection.hpp"
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>

namespace iconstat {
class ApiClient final {
public:
    using FetchCallback = geode::Function<void(geode::Result<std::optional<Collection>>) >;
    using ActionCallback = geode::Function<void(geode::Result<std::string>)>;

    void fetch(int accountID, FetchCallback callback);
    void synchronize(bool interactive, ActionCallback callback = {});
    void deletePublished(ActionCallback callback = {});
    bool isActionInFlight() const { return m_actionInFlight; }
    static void clearCache();

private:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_fetchTask;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_actionWebTask;
    geode::async::TaskHolder<geode::Result<std::string>> m_actionAuthTask;
    bool m_actionInFlight = false;

    void authenticatedRequest(std::string method, Collection collection, ActionCallback callback);
    static std::string cacheKey(int accountID);
};

ApiClient& api();
} // namespace iconstat
