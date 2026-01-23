// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_SKIN_TREE_H__
#define __FG_SKIN_TREE_H__

#include "fgStyle.h"
#include "fgElement.h"

struct _FG_SKIN;
struct _FG_SKIN_BASE;

typedef struct _FG_SKIN_ELEMENT {
  const char* type;
  fgTransform transform;
  fgMsgType units;
  fgFlag flags;
  fgStyle style; // style overrides
  struct _FG_SKIN* skin; // Skin override (cannot be passed as a message because this blows up absolutely everything)
  int order;
} fgSkinElement;

struct _FG_SKIN_LAYOUT;
typedef fgDeclareVector(struct _FG_SKIN_LAYOUT, SkinLayout) fgVectorSkinLayout;

typedef struct _FG_SKIN_TREE {
  struct fgStylePair {
    FG_UINT map;
    fgStyle style;

#ifdef  __cplusplus
    std::strong_ordering operator <=>(const fgStylePair& r) const {
      uint8_t lb = bun::BitCount(map);
      uint8_t rb = bun::BitCount(r.map);
      auto ret = rb <=> lb;
      return ret == 0 ? (r.map <=> map) : ret;
    }
    bool operator==(const fgStylePair& r) const {
      return map == r.map;
    }
#endif
  };

  fgVectorSkinLayout children;
  fgDeclareVector(struct fgStylePair, StylePair) styles;
  size_t stylemask;

#ifdef  __cplusplus
  FG_DLLEXPORT size_t AddChild(const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
  FG_DLLEXPORT char RemoveChild(FG_UINT child);
  FG_DLLEXPORT struct _FG_SKIN_LAYOUT* GetChild(FG_UINT child) const;
  FG_DLLEXPORT FG_UINT AddStyle(const char* name);
  FG_DLLEXPORT char RemoveStyle(FG_UINT style);
  FG_DLLEXPORT fgStyle* GetStyle(FG_UINT style) const;
#endif
} fgSkinTree;

typedef struct _FG_SKIN_LAYOUT {
#ifdef  __cplusplus
  _FG_SKIN_LAYOUT(const _FG_SKIN_LAYOUT&);
  _FG_SKIN_LAYOUT(_FG_SKIN_LAYOUT&&) = default;
  _FG_SKIN_LAYOUT(const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
  ~_FG_SKIN_LAYOUT();

  _FG_SKIN_LAYOUT& operator=(_FG_SKIN_LAYOUT&&) = default;
  _FG_SKIN_LAYOUT& operator=(const _FG_SKIN_LAYOUT&);
  inline std::strong_ordering operator<=>(const _FG_SKIN_LAYOUT& r) const {
    return element.order <=> r.element.order;
  }
  inline bool operator==(const _FG_SKIN_LAYOUT& r) const {
    return operator<=>(r) == 0;
  }

#endif

  fgSkinElement element;
  fgSkinTree tree;
  fgElement* instance; // Instance of this skin element. Skin elements cannot be assigned skins, so this only needs to apply the style overrides.
} fgSkinLayout;

FG_EXTERN void fgSkinElement_Init(fgSkinElement* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN void fgSkinElement_InitCopy(fgSkinElement* self, const fgSkinElement* from);
FG_EXTERN void fgSkinElement_ResolveCopy(fgSkinElement* self, struct _FG_SKIN_BASE* parent);
FG_EXTERN void fgSkinElement_Destroy(fgSkinElement* self);

FG_EXTERN void fgSkinTree_Init(fgSkinTree* self);
FG_EXTERN void fgSkinTree_InitCopy(fgSkinTree* self, const fgSkinTree* from);
FG_EXTERN void fgSkinTree_ResolveCopy(fgSkinTree* self, struct _FG_SKIN_BASE* parent);
FG_EXTERN void fgSkinTree_Destroy(fgSkinTree* self);
FG_EXTERN size_t fgSkinTree_AddChild(fgSkinTree* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN char fgSkinTree_RemoveChild(fgSkinTree* self, FG_UINT child);
FG_EXTERN fgSkinLayout* fgSkinTree_GetChild(const fgSkinTree* self, FG_UINT child);
FG_EXTERN FG_UINT fgSkinTree_AddStyle(fgSkinTree* self, const char* name);
FG_EXTERN char fgSkinTree_RemoveStyle(fgSkinTree* self, FG_UINT style);
FG_EXTERN fgStyle* fgSkinTree_GetStyle(const fgSkinTree* self, FG_UINT style);

FG_EXTERN void fgSkinLayout_Init(fgSkinLayout* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN void fgSkinLayout_InitCopy(fgSkinLayout* self, const fgSkinLayout* from);
FG_EXTERN void fgSkinLayout_Destroy(fgSkinLayout* self);

#endif