
#ifndef X
#error X must be defined. This macro corresponds to known effects with manually specified SkSL source.
#endif

#ifndef Y
#error Y must be defined. This macro corresponds to linear effects.
#endif

X(BlurFilter_MixEffect,                                     0 )
X(EdgeExtensionEffect,                                      1 )
X(GainmapEffect,                                            2 )
X(KawaseBlurDualFilter_LowSampleBlurEffect,                 3 )
X(KawaseBlurDualFilter_HighSampleBlurEffect,                4 )
X(KawaseBlurDualFilterV2_QuarterResDownSampleBlurEffect,    5 )
X(KawaseBlurDualFilterV2_HalfResDownSampleBlurEffect,       6 )
X(KawaseBlurDualFilterV2_UpSampleBlurEffect,                7 )
X(KawaseBlurEffect,                                         8 )
X(LutEffect,                                                9 )
X(MouriMap_CrossTalkAndChunk16x16Effect,                    10)
X(MouriMap_Chunk8x8Effect,                                  11)
X(MouriMap_BlurEffect,                                      12)
X(MouriMap_TonemapEffect,                                   13)
X(StretchEffect,                                            14)
Y(UNKNOWN__SRGB__false__UNKNOWN__Shader,                    15)
Y(BT2020_ITU_PQ__BT2020__false__UNKNOWN__Shader,            16)
Y(0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,         17)
Y(V0_SRGB__V0_SRGB__true__UNKNOWN__Shader,                  18)
Y(0x188a0000__V0_SRGB__true__0x9010000__Shader,             19)
X(BoxShadowEffect,                                          20)

 // IMPORTANT: Do not change the order of existing effects in this list.
 //
 // This list populates skgpu::graphite::ContextOptions::fUserDefinedKnownRuntimeEffects.
 //
 // The index of an effect in this array is used when serializing a graphics
 // pipeline description. Altering the order will break the ability to reuse serialized
 // pipeline descriptions from older builds.
 //
 // To preserve compatibility, always add new effects to the end.

#undef X
#undef Y