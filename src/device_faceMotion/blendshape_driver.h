#pragma once

/** \file blendshape_driver.h
 *
 *  Optional second output path: writes parsed blendshape weights directly
 *  to an FBModel's animatable double properties whose names match the
 *  52 ARKit camelCase names (e.g. "eyeBlinkLeft", "jawOpen").
 *
 *  This works when the target model exposes its Blendshape deformer
 *  channels as top-level properties (the common path for FBX-imported
 *  face meshes). The lookup is performed once on Rebind() and cached.
 */

#include "protocol.h"

class FBModel;
class FBProperty;

namespace mobufacemotion {

class BlendshapeDriver
{
public:
    /// Rebind to `target`. Safe to call every frame; a rebuild only happens
    /// when the target pointer differs from the previously cached one.
    /// Pass nullptr to clear.
    void Rebind(FBModel* target);

    /// Write the present blendshape values from `frame` into the cached
    /// target properties. `valueMult` is applied (frame values are 0..100;
    /// MoBu blendshape channels are typically 0..100 already, so 1.0 is the
    /// usual choice — set to 0.01 if your target expects 0..1).
    void Apply(const ParsedFrame& frame, double valueMult) const;

    FBModel* Target() const { return mCachedTarget; }

private:
    FBModel*    mCachedTarget = nullptr;
    FBProperty* mProps[kBlendshapeCount] = {};
};

} // namespace mobufacemotion
