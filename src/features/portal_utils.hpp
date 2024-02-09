#pragma once

#include <string>
#include "ent_props.hpp"

class IClientEntity;
class Vector;
class QAngle;

class PortalUtilsFeature : public FeatureWrapper<PortalUtilsFeature>
{
public:
    void calculateSGPosition(const char* portalStr, Vector& new_player_origin, QAngle& new_player_angles);
    void calculateAGPosition(const char* portalStr, Vector& new_player_origin, QAngle& new_player_angles);
    PlayerField<int> m_hPortalEnvironment;
protected:
    virtual bool ShouldLoadFeature() override;

    virtual void InitHooks() override;

    virtual void LoadFeature() override;

    virtual void UnloadFeature() override;
};

extern PortalUtilsFeature feat_portal_utils;



