#ifndef _wxjs_gui_mdi_child_h
#define _wxjs_gui_mdi_child_h

namespace wxjs
{
  namespace gui
  {
    class MDIChildFrame : public ApiWrapper<MDIChildFrame, wxMDIChildFrame>
                        , public wxMDIChildFrame
    {
    public:

      MDIChildFrame() : wxMDIChildFrame()
      {
      }

      virtual ~MDIChildFrame() {}

      virtual wxToolBar* OnCreateToolBar(long style, 
                                         wxWindowID id,
                                         const wxString& name);

      static bool AddProperty(wxMDIChildFrame *p, 
                              JSContext *cx, 
                              JSObject *obj, 
                              const wxString &prop, 
                              jsval *vp);
      static bool DeleteProperty(wxMDIChildFrame *p, 
                                 JSContext* cx, 
                                 JSObject* obj, 
                                 const wxString &prop);

      static wxMDIChildFrame* Construct(JSContext* cx, 
                                        JSObject* obj, 
                                        uintN argc, 
                                        jsval* argv, 
                                        bool constructing);
      
      WXJS_DECLARE_METHOD_MAP()
      WXJS_DECLARE_METHOD(create)
      WXJS_DECLARE_METHOD(activate)
      WXJS_DECLARE_METHOD(maximize)
      WXJS_DECLARE_METHOD(restore)
                     
    private:                     
    };
  }
};

#endif // _wxjs_gui_mdi_child_h
