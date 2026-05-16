/** \file blendshape_driver.cxx */

#include "blendshape_driver.h"

#include <cctype>
#include <cstring>

#include <fbsdk/fbsdk.h>

namespace mobufacemotion {

namespace {

bool IEqual(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (std::tolower(static_cast<unsigned char>(*a)) !=
            std::tolower(static_cast<unsigned char>(*b)))
            return false;
        ++a; ++b;
    }
    return *a == 0 && *b == 0;
}

FBProperty* FindMatching(FBModel* model, const char* arkitName)
{
    // Direct case-sensitive hit first (cheap path).
    if (FBProperty* p = model->PropertyList.Find(arkitName))
        return p;

    // Case-insensitive scan: covers shape channels that may have been imported
    // with different casing (some DCC pipelines title-case them).
    const int n = model->PropertyList.GetCount();
    for (int i = 0; i < n; ++i)
    {
        FBProperty* p = model->PropertyList[i];
        if (p && IEqual(p->GetName(), arkitName))
            return p;
    }
    return nullptr;
}

} // namespace

void BlendshapeDriver::Rebind(FBModel* target)
{
    if (target == mCachedTarget) return;

    mCachedTarget = target;
    std::memset(mProps, 0, sizeof(mProps));

    if (!target) return;

    for (uint32_t i = 0; i < kBlendshapeCount; ++i)
        mProps[i] = FindMatching(target, kArkitBlendshapes[i]);
}

void BlendshapeDriver::Apply(const ParsedFrame& frame, double valueMult) const
{
    if (!mCachedTarget) return;

    for (uint32_t i = 0; i < kBlendshapeCount; ++i)
    {
        if (!mProps[i]) continue;
        if (!(frame.blendshapeMask & (uint64_t(1) << i))) continue;

        const double v = frame.blendshape[i] * valueMult;
        mProps[i]->SetData(const_cast<double*>(&v));
    }
}

} // namespace mobufacemotion
