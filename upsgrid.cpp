#include <wx/wxprec.h>
#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif
#include <wx/grid.h>
#include "upsgrid.h"

wxBEGIN_EVENT_TABLE(UPSGrid, wxGrid)
EVT_GRID_CELL_LEFT_CLICK(UPSGrid::on_cell_left_click)
wxEND_EVENT_TABLE()

UPSGrid::UPSGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name)
  : wxGrid(parent, id, pos, size, style, name) {
}

void UPSGrid::on_cell_left_click(wxGridEvent &event) {
  m_waitForSlowClick = true;
  SetGridCursor(event.GetRow(), event.GetCol());
  event.Skip();
}
