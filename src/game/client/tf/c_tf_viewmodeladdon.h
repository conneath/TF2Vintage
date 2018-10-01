//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_TF_VIEWMODELENT_H
#define C_TF_VIEWMODELENT_H

#include "c_baseviewmodel.h"

class C_TFViewModel;

class C_ViewmodelAttachmentModel : public C_BaseViewModel
{
	DECLARE_CLASS( C_ViewmodelAttachmentModel, C_BaseViewModel );
public:

	virtual int	InternalDrawModel(int flags);
	virtual bool OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	virtual int	DrawModel( int flags );

	virtual int DrawOverriddenViewmodel( int flags );
	virtual void StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	void SetViewmodel( C_TFViewModel *vm );

	CHandle< C_TFViewModel > m_viewmodel;

	virtual bool			IsViewModel() const { return true; }

	virtual RenderGroup_t	GetRenderGroup( void ) { return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT; }

};
#endif