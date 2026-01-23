// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_LAYOUT_H__
#define __FG_LAYOUT_H__

#include "fgSkin.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct kh_fgLayoutMap_s;
struct _FG_CLASS_LAYOUT;
typedef fgDeclareVector(struct _FG_CLASS_LAYOUT, ClassLayout) fgVectorClassLayout;

struct _FG_KEY_VALUE {
#ifdef  __cplusplus
  BUN_FORCEINLINE _FG_KEY_VALUE(const _FG_KEY_VALUE& r) {
    key = fgCopyText(r.key, __FILE__, __LINE__);
    value = fgCopyText(r.value, __FILE__, __LINE__);
  }
  _FG_KEY_VALUE(_FG_KEY_VALUE&&) = default;
  BUN_FORCEINLINE _FG_KEY_VALUE(const char* k, const char* v) {
    key = fgCopyText(k, __FILE__, __LINE__);
    value = fgCopyText(v, __FILE__, __LINE__);
  }

  BUN_FORCEINLINE ~_FG_KEY_VALUE()
  {
    fgFreeText(key, __FILE__, __LINE__);
    if (value)
      fgFreeText(value, __FILE__, __LINE__);
  }
  _FG_KEY_VALUE& operator=(_FG_KEY_VALUE&&) = default;
  BUN_FORCEINLINE _FG_KEY_VALUE& operator=(const _FG_KEY_VALUE& r) {
    key = fgCopyText(r.key, __FILE__, __LINE__);
    value = fgCopyText(r.value, __FILE__, __LINE__);
  }
#endif
  const char* key;
  const char* value; 
};
typedef fgDeclareVector(struct _FG_KEY_VALUE, KeyValue) fgVectorKeyValue;

typedef struct _FG_CLASS_LAYOUT {
  fgSkinElement element;
  const char* name;
  const char* id;
  fgVectorClassLayout children; // Type: fgClassLayout
  fgVectorKeyValue userdata; // Custom userdata from unknown attributes
  FG_UINT userid;

#ifdef  __cplusplus
  _FG_CLASS_LAYOUT(const _FG_CLASS_LAYOUT&);
  _FG_CLASS_LAYOUT(_FG_CLASS_LAYOUT&&) = default;
  _FG_CLASS_LAYOUT(const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
  ~_FG_CLASS_LAYOUT();

  _FG_CLASS_LAYOUT& operator=(_FG_CLASS_LAYOUT&&) = default;
  _FG_CLASS_LAYOUT& operator=(const _FG_CLASS_LAYOUT&);
  inline std::strong_ordering operator<=>(const _FG_CLASS_LAYOUT& r) const {
    return element.order <=> r.element.order;
  }
  inline bool operator==(const _FG_CLASS_LAYOUT& r) const {
    return operator<=>(r) == 0;
  }

  FG_DLLEXPORT void AddUserString(const char* key, const char* value);
  FG_DLLEXPORT FG_UINT AddChild(const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
  FG_DLLEXPORT bool RemoveChild(FG_UINT child);
  FG_DLLEXPORT struct _FG_CLASS_LAYOUT* GetChild(FG_UINT child) const;
#endif
} fgClassLayout;

typedef struct _FG_LAYOUT {
  fgSkinBase base;
  fgSkin* skin; // if not null, specifies the skin to assign to this layout.
  fgVectorClassLayout children; // Type: fgClassLayout
  struct kh_fgLayoutMap_s* sublayouts;

#ifdef  __cplusplus
  FG_DLLEXPORT FG_UINT AddChild(const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
  FG_DLLEXPORT bool RemoveChild(FG_UINT child);
  FG_DLLEXPORT fgClassLayout* GetChild(FG_UINT child) const;
  FG_DLLEXPORT struct _FG_LAYOUT* AddLayout(const char* name);
  FG_DLLEXPORT bool RemoveLayout(const char* name);
  FG_DLLEXPORT struct _FG_LAYOUT* GetLayout(const char* name) const;
  FG_DLLEXPORT void IterateLayouts(void* p, void(*f)(void*, struct _FG_LAYOUT*, const char*));

  //FG_DLLEXPORT void LoadFileUBJSON(const char* file);
  //FG_DLLEXPORT void LoadUBJSON(const char* data, FG_UINT length);
  //FG_DLLEXPORT void SaveFileUBJSON(const char* file);
  FG_DLLEXPORT bool LoadFileXML(const char* file);
  FG_DLLEXPORT bool LoadXML(const char* data, FG_UINT length, const char* path = 0);
  FG_DLLEXPORT void SaveFileXML(const char* file);
#endif
} fgLayout;

FG_EXTERN void fgLayout_Init(fgLayout* self, const char* name, const char* path);
FG_EXTERN void fgLayout_InitCopy(fgLayout* self, const fgLayout* from, fgSkinBase* parent);
FG_EXTERN void fgLayout_Destroy(fgLayout* self);
FG_EXTERN FG_UINT fgLayout_AddChild(fgLayout* self, const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN char fgLayout_RemoveChild(fgLayout* self, FG_UINT child);
FG_EXTERN fgClassLayout* fgLayout_GetChild(const fgLayout* self, FG_UINT child);
FG_EXTERN fgLayout* fgLayout_AddLayout(fgLayout* self, const char* name);
FG_EXTERN char fgLayout_RemoveLayout(fgLayout* self, const char* name);
FG_EXTERN fgLayout* fgLayout_GetLayout(const fgLayout* self, const char* name);
FG_EXTERN void fgLayout_IterateLayouts(fgLayout* self, void* p, void(*f)(void*, fgLayout*, const char*));

//FG_EXTERN void fgLayout_LoadFileUBJSON(fgLayout* self, const char* file);
//FG_EXTERN void fgLayout_LoadUBJSON(fgLayout* self, const char* data, FG_UINT length, const char* path);
//FG_EXTERN void fgLayout_SaveFileUBJSON(fgLayout* self, const char* file);
FG_EXTERN char fgLayout_LoadFileXML(fgLayout* self, const char* file);
FG_EXTERN char fgLayout_LoadXML(fgLayout* self, const char* data, FG_UINT length, const char* path);
FG_EXTERN void fgLayout_SaveFileXML(fgLayout* self, const char* file);

FG_EXTERN void fgClassLayout_Init(fgClassLayout* self, const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN void fgClassLayout_InitCopy(fgClassLayout* self, const fgClassLayout* from, fgSkinBase* parent);
FG_EXTERN void fgClassLayout_Destroy(fgClassLayout* self);
FG_EXTERN void fgClassLayout_AddUserString(fgClassLayout* self, const char* key, const char* value);
FG_EXTERN FG_UINT fgClassLayout_AddChild(fgClassLayout* self, const char* type, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units, int order);
FG_EXTERN char fgClassLayout_RemoveChild(fgClassLayout* self, FG_UINT child);
FG_EXTERN fgClassLayout* fgClassLayout_GetChild(const fgClassLayout* self, FG_UINT child);

FG_EXTERN size_t fgDefaultLayout(fgElement* self, const FG_Msg* msg, AbsVec* dim);
FG_EXTERN size_t fgDistributeLayout(fgElement* self, const FG_Msg* msg, fgFlag flags, AbsVec* dim);
FG_EXTERN size_t fgTileLayout(fgElement* self, const FG_Msg* msg, fgFlag flags, AbsVec* area, AbsVec spacing);

#ifdef  __cplusplus
}
#endif

#endif