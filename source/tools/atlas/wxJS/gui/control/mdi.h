#ifndef _wxjs_gui_mdi_h
#define _wxjs_gui_mdi_h

namespace wxjs
{
  namespace gui
  {
    class MDIParentFrame : public ApiWrapper<MDIParentFrame, wxMDIParentFrame>
                         , public wxMDIParentFrame
    {
    public:

      MDIParentFrame() : wxMDIParentFrame()
      {
      }

      virtual ~MDIParentFrame() {}

      virtual wxToolBar* OnCreateToolBar(long style, 
                                         wxWindowID id,
                                         const wxString& name);
      static bool AddProperty(wxMDIParentFrame *p, 
                              JSContext *cx, 
                              JSObject *obj, 
                              const wxString &prop, 
                              jsval *vp);
      static bool DeleteProperty(wxMDIParentFrame *p, 
                                 JSContext* cx, 
                                 JSObject* obj, 
                                 const wxString &prop);
      static wxMDIParentFrame* Construct(JSContext* cx, 
                                         JSObject* obj, 
                                         uintN argc, 
                                         jsval* argv, 
                                         bool constructing);

      static bool GetProperty(wxMDIParentFrame* p, 
                              JSContext* cx, JSObject* obj, int id, jsval* vp);
      
      WXJS_DECLARE_PROPERTY_MAP()
      enum
      {
          P_ACTIVE_CHILD
      };
      
      WXJS_DECLARE_METHOD_MAP()
      WXJS_DECLARE_METHOD(create)
      WXJS_DECLARE_METHOD(activateNext)
      WXJS_DECLARE_METHOD(activatePrevious)
      WXJS_DECLARE_METHOD(arrangeIcons)
      WXJS_DECLARE_METHOD(cascade)
      WXJS_DECLARE_METHOD(tile)
                     
    private:                     
    };
  }
};

#endif // _wxjs_gui_mdi_h
