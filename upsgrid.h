class UPSGrid : public wxGrid {
  public:
    UPSGrid(wxWindow *parent, wxWindowID id,
        const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize,
        long style=wxWANTS_CHARS, const wxString &name=wxPanelNameStr);
    void deselect_cells();

  private:
    void on_cell_left_click(wxGridEvent &event);

    wxDECLARE_EVENT_TABLE();
};
