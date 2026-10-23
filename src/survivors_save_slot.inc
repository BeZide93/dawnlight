// File info is shared by file select, copy destinations and the native save
// menu. Read the actual slot, never the most recently loaded s_records.
constexpr u64 kSaveMetaTag=MULTI_CHAR('dl_meta');
constexpr std::array<u64,20> kSaveHeartTags{
    MULTI_CHAR('hear_20'),MULTI_CHAR('hear_21'),MULTI_CHAR('hear_22'),MULTI_CHAR('hear_23'),MULTI_CHAR('hear_24'),
    MULTI_CHAR('hear_25'),MULTI_CHAR('hear_26'),MULTI_CHAR('hear_27'),MULTI_CHAR('hear_28'),MULTI_CHAR('hear_29'),
    MULTI_CHAR('hear_30'),MULTI_CHAR('hear_31'),MULTI_CHAR('hear_32'),MULTI_CHAR('hear_33'),MULTI_CHAR('hear_34'),
    MULTI_CHAR('hear_35'),MULTI_CHAR('hear_36'),MULTI_CHAR('hear_37'),MULTI_CHAR('hear_38'),MULTI_CHAR('hear_39')};

void after_survivors_save_slot(ModContext*,void* args,void* result,void*) {
    auto* info=mods::arg<dFile_info_c*>(args,0);
    if(!info || !info->mFileInfo.Scr)return;
    auto* screen=info->mFileInfo.Scr;
    auto* label=static_cast<J2DTextBox*>(screen->search(kSaveMetaTag));
    if(label){label->hide();label->setUserInfo(0);}
    const auto slot=mods::arg<u8>(args,3);
    const int status=result?*static_cast<int*>(result):-1;
    if(!s_active || slot>=3 || (status!=0 && status!=2) || !svc_save || !svc_save->peek_blob)return;

    std::array<uint8_t,Records::byte_count> bytes{};size_t size=bytes.size();
    Records records;
    const auto read=svc_save->peek_blob(mod_ctx,slot,kBlob,bytes.data(),&size);
    const bool known=read==MOD_UNAVAILABLE ||
        (read==MOD_OK && size<=bytes.size() && records.decode(std::span(bytes.data(),size)));
    char text[32];
    if(known)std::snprintf(text,sizeof(text),"Meta Level %u",records.level());
    else std::snprintf(text,sizeof(text),"Meta Level ?");

    if(!info->mFileInfo.field_0x10 || !info->mFileInfo.mFont ||
        !screen->search(MULTI_CHAR('w_ptim01')))return;
    std::array<J2DPane*,20> hearts{};
    for(unsigned i=0;i<hearts.size();++i) {
        hearts[i]=screen->search(kSaveHeartTags[i]);
        if(!hearts[i])return;
    }
    if(!label) {
        label=JKR_NEW J2DTextBox(kSaveMetaTag,JGeometry::TBox2<f32>(0,0,1,1),nullptr,"",sizeof(text),HBIND_LEFT,VBIND_CENTER);
        if(!label)return;
        // Keep this native-owned, hidden storage pane OUTSIDE mDatBase's tree:
        // CPaneMgrAlpha cached that subtree's child count during screenSet().
        if(!screen->appendChild(label)){JKR_DELETE(label);return;}
        label->setFont(info->mFileInfo.mFont);
    }
    label->hide(); // Draw explicitly after the native slot has its final matrices.
    label->setString(sizeof(text),text); // Native setter copies literal text.
    if(!label->getStringPtr())return;
    label->setUserInfo(1);
    for(auto* heart:hearts)heart->hide();
}

void after_survivors_save_slot_draw(ModContext*,void* args,void*,void*) {
    auto* info=mods::arg<dDlst_FileInfo_c*>(args,0);
    if(!s_active || !info || !info->Scr)return;
    auto* label=static_cast<J2DTextBox*>(info->Scr->search(kSaveMetaTag));
    if(!label || !label->getUserInfo() || !label->getStringPtr())return;
    auto* style=static_cast<J2DTextBox*>(info->Scr->search(MULTI_CHAR('w_ptim01')));
    if(!style || !style->mColorAlpha)return;
    for(J2DPane* pane=style;pane;pane=pane->getParentPane())if(!pane->isVisible())return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    // dDlst_FileInfo_c::draw has now applied mBasePane to Nm_02 and drawn the
    // tree. Even hidden hearts have fresh global matrices. Use the same final
    // vertices as their native pictures, including replacement layouts.
    JGeometry::TBox2<f32> area;bool first=true;
    for(auto tag:kSaveHeartTags) {
        auto* heart=info->Scr->search(tag);if(!heart)return;
        for(unsigned corner=0;corner<4;++corner) {
            const auto point=heart->getGlbVtx(corner);
            if(!std::isfinite(point.x)||!std::isfinite(point.y))return;
            if(first){area=JGeometry::TBox2<f32>(point.x,point.y,point.x,point.y);first=false;}
            else {area.i.x=std::min(area.i.x,point.x);area.i.y=std::min(area.i.y,point.y);
                  area.f.x=std::max(area.f.x,point.x);area.f.y=std::max(area.f.y,point.y);}
        }
    }
    if(area.getWidth()<=0 || area.getHeight()<=0)return;
    J2DTextBox::TFontSize font;style->getFontSize(font);
    const auto& m=style->mGlobalMtx;
    font.mSizeX*=std::hypot(m[0][0],m[1][0]);font.mSizeY*=std::hypot(m[0][1],m[1][1]);
    const float scale=std::min({1.f,area.getHeight()/std::max(1.f,font.mSizeY),
        area.getWidth()/std::max(1.f,font.mSizeX*float(std::strlen(label->getStringPtr())))});
    label->setFontSize(font.mSizeX*scale,font.mSizeY*scale);
    label->setFontColor(style->getCharColor(),style->getGradColor());
    label->mBounds=JGeometry::TBox2<f32>(0,0,area.getWidth(),area.getHeight());
    MTXIdentity(label->mGlobalMtx);
    label->mGlobalMtx[0][3]=area.i.x;label->mGlobalMtx[1][3]=area.i.y;
    label->mColorAlpha=style->mColorAlpha;
    // drawSelf uses the supplied native graf matrix and does not show the pane
    // during the next ordinary tree traversal. No per-frame allocation or reads.
    label->drawSelf(0,0,&graf->mPosMtx);
}
