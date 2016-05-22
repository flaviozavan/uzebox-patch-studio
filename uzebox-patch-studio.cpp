#include <wx/wxprec.h>
#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif
#include <wx/aboutdlg.h>
#include <wx/treectrl.h>
#include <wx/regex.h>
#include <wx/grid.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/sound.h>
#include <algorithm>
#include <map>
#include <set>
#include <SDL.h>
#include <SDL_mixer.h>
#include <regex>
#include "upsgrid.h"
#include "filereader.h"
#include "patchdata.h"
#include "structdata.h"
#include "uglyicon.xpm"

#define MIN_CLIENT_HEIGHT 400

class UPSApp: public wxApp {
  public:
    virtual bool OnInit();
    virtual int OnExit();
};

class UPSFrame: public wxFrame {
  public:
    UPSFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    void open_file(const wxString &path);

  private:
    void on_new(wxCommandEvent &event);
    void on_exit(wxCommandEvent &event);
    void on_about(wxCommandEvent &event);
    void on_data_tree_label_edit(wxTreeEvent &event);
    void on_data_tree_changed(wxTreeEvent &event);
    void on_new_patch(wxCommandEvent &event);
    void on_new_struct(wxCommandEvent &event);
    void on_rename(wxCommandEvent &event);
    void on_data_tree_label_edit_end(wxTreeEvent &event);
    void on_new_command(wxCommandEvent &event);
    void on_delete_command(wxCommandEvent &event);
    void on_up_command(wxCommandEvent &event);
    void on_down_command(wxCommandEvent &event);
    void on_clone_command(wxCommandEvent &event);
    void on_cell_changed(wxGridEvent &event);
    void on_save_as(wxCommandEvent &event);
    void on_save(wxCommandEvent &event);
    void on_open(wxCommandEvent &event);
    void on_play(wxCommandEvent &event);
    void on_loop(wxCommandEvent &event);
    void on_stop(wxCommandEvent &event);
    void on_stop_all(wxCommandEvent &event);
    void on_remove(wxCommandEvent &event);
    void on_clone_data(wxCommandEvent &event);
    void on_sync(wxCommandEvent &event);

    bool validate_var_name(const wxString &name);

    wxString get_next_data_name(const wxString &base, bool try_bare=false);
    wxTreeItemId find_data(const wxTreeItemId &item, const wxString &name);
    int add_patch_command(const wxString &delay="0",
        const wxString &command=command_choices[0],
        const wxString &param="0",
        int pos=-1);
    int add_struct_command(const wxString &type=_("Wave"),
        const wxString &pcm="NULL",
        const wxString &patch=wxEmptyString,
        const wxString &loop_start="0",
        const wxString &loop_end="0",
        int pos=-1);
    void update_patch_data(const wxTreeItemId &item);
    void read_patch_data(const wxTreeItemId &item);
    void update_patch_row_colors(int row);
    void save_to_file(const wxString &path);
    void clear();
    void update_struct_row_colors(int row);
    void update_struct_data(const wxTreeItemId &item);
    void read_struct_data(const wxTreeItemId &item);
    void replace_patch_in_structs(const wxString &src, const wxString &dst);
    void update_layout();

    wxRegEx valid_var_name;
    wxTreeItemId data_tree_root;
    wxTreeItemId data_tree_patches;
    wxTreeItemId data_tree_structs;
    wxTreeCtrl *data_tree;
    UPSGrid *patch_grid;
    UPSGrid *struct_grid;
    wxBoxSizer *top_sizer;
    wxBoxSizer *right_sizer;
    wxString current_file_path;
    std::set<wxString> patch_names = {wxT("NULL")};

    static const std::map<wxString, std::pair<long, long>> limits;
    static const wxString command_choices[16];
    static const wxString type_choices[3];
    static const std::map<wxString, long> choice_values;
    static const std::map<wxString, long> command_ids;

    wxDECLARE_EVENT_TABLE();
};

enum {
  ID_PLAY = 1,
  ID_LOOP,
  ID_STOP,
  ID_STOP_ALL,
  ID_SYNC,
  ID_DATA_TREE,
  ID_NEW_PATCH,
  ID_NEW_STRUCT,
  ID_RENAME_DATA,
  ID_REMOVE_DATA,
  ID_CLONE_DATA,
  ID_PATCH_GRID,
  ID_STRUCT_GRID,
  ID_NEW_COMMAND,
  ID_DELETE_COMMAND,
  ID_UP_COMMAND,
  ID_DOWN_COMMAND,
  ID_CLONE_COMMAND,
};

wxBEGIN_EVENT_TABLE(UPSFrame, wxFrame)
  EVT_MENU(wxID_NEW, UPSFrame::on_new)
  EVT_MENU(wxID_EXIT, UPSFrame::on_exit)
  EVT_MENU(wxID_ABOUT, UPSFrame::on_about)
  EVT_TREE_BEGIN_LABEL_EDIT(ID_DATA_TREE, UPSFrame::on_data_tree_label_edit)
  EVT_TREE_SEL_CHANGED(ID_DATA_TREE, UPSFrame::on_data_tree_changed)
  EVT_BUTTON(ID_NEW_PATCH, UPSFrame::on_new_patch)
  EVT_BUTTON(ID_NEW_STRUCT, UPSFrame::on_new_struct)
  EVT_BUTTON(ID_RENAME_DATA, UPSFrame::on_rename)
  EVT_TREE_END_LABEL_EDIT(ID_DATA_TREE, UPSFrame::on_data_tree_label_edit_end)
  EVT_BUTTON(ID_NEW_COMMAND, UPSFrame::on_new_command)
  EVT_BUTTON(ID_DELETE_COMMAND, UPSFrame::on_delete_command)
  EVT_BUTTON(ID_UP_COMMAND, UPSFrame::on_up_command)
  EVT_BUTTON(ID_DOWN_COMMAND, UPSFrame::on_down_command)
  EVT_BUTTON(ID_CLONE_COMMAND, UPSFrame::on_clone_command)
  EVT_GRID_CELL_CHANGED(UPSFrame::on_cell_changed)
  EVT_MENU(wxID_SAVEAS, UPSFrame::on_save_as)
  EVT_MENU(wxID_SAVE, UPSFrame::on_save)
  EVT_MENU(wxID_OPEN, UPSFrame::on_open)
  EVT_MENU(ID_PLAY, UPSFrame::on_play)
  EVT_MENU(ID_LOOP, UPSFrame::on_loop)
  EVT_MENU(ID_STOP, UPSFrame::on_stop)
  EVT_MENU(ID_STOP_ALL, UPSFrame::on_stop_all)
  EVT_MENU(ID_SYNC, UPSFrame::on_sync)
  EVT_BUTTON(ID_REMOVE_DATA, UPSFrame::on_remove)
  EVT_BUTTON(ID_CLONE_DATA, UPSFrame::on_clone_data)
wxEND_EVENT_TABLE()
wxIMPLEMENT_APP(UPSApp);

bool UPSApp::OnInit() {
  UPSFrame *frame = new UPSFrame(_("Uzebox Patch Studio"),
      wxPoint(50, 50), wxSize(600, 400));
  frame->SetIcon(uglyicon);
  frame->Show(true);

  if (SDL_Init(SDL_INIT_AUDIO) == -1
      || Mix_OpenAudio(SAMPLE_RATE, AUDIO_U8, 1, 4096) == -1) {
    wxMessageDialog(frame, SDL_GetError(),
        _("SDL Error"), wxOK | wxICON_ERROR).ShowModal();
    return false;
  }

  if (argc > 1) {
    frame->open_file(argv[1]);
  }

  return true;
}

int UPSApp::OnExit() {
  Mix_CloseAudio();
  SDL_Quit();

  return 0;
}

const std::map<wxString, std::pair<long, long>> UPSFrame::limits = {
  {_("ENV_SPEED"), std::make_pair(-128, 127)},
  {_("NOISE_PARAMS"), std::make_pair(0, 127)},
  {_("WAVE"), std::make_pair(0, NUM_WAVES-1)},
  {_("NOTE_UP"), std::make_pair(0, 255)},
  {_("NOTE_DOWN"), std::make_pair(0, 255)},
  {_("NOTE_CUT"), std::make_pair(0, 0)},
  {_("NOTE_HOLD"), std::make_pair(0, 0)},
  {_("ENV_VOL"), std::make_pair(0, 255)},
  {_("PITCH"), std::make_pair(0, 126)},
  {_("TREMOLO_LEVEL"), std::make_pair(0, 255)},
  {_("TREMOLO_RATE"), std::make_pair(0, 255)},
  {_("SLIDE"), std::make_pair(-128, 127)},
  {_("SLIDE_SPEED"), std::make_pair(0, 255)},
  {_("LOOP_START"), std::make_pair(0, 255)},
  {_("LOOP_END"), std::make_pair(0, 255)},
  {_("PATCH_END"), std::make_pair(0, 0)},
};

const wxString UPSFrame::command_choices[16] = {
  _("ENV_SPEED"),
  _("NOISE_PARAMS"),
  _("WAVE"),
  _("NOTE_UP"),
  _("NOTE_DOWN"),
  _("NOTE_CUT"),
  _("NOTE_HOLD"),
  _("ENV_VOL"),
  _("PITCH"),
  _("TREMOLO_LEVEL"),
  _("TREMOLO_RATE"),
  _("SLIDE"),
  _("SLIDE_SPEED"),
  _("LOOP_START"),
  _("LOOP_END"),
  _("PATCH_END"),
};

const wxString UPSFrame::type_choices[3] = {
  _("Wave"),
  _("Noise"),
  _("PCM"),
};

const std::map<wxString, long> UPSFrame::choice_values = {
  {_("Wave"), 0},
  {_("Noise"), 1},
  {_("PCM"), 2},
};

const std::map<wxString, long> UPSFrame::command_ids = {
  {_("ENV_SPEED"), 0},
  {_("NOISE_PARAMS"), 1},
  {_("WAVE"), 2},
  {_("NOTE_UP"), 3},
  {_("NOTE_DOWN"), 4},
  {_("NOTE_CUT"), 5},
  {_("NOTE_HOLD"), 6},
  {_("ENV_VOL"), 7},
  {_("PITCH"), 8},
  {_("TREMOLO_LEVEL"), 9},
  {_("TREMOLO_RATE"), 10},
  {_("SLIDE"), 11},
  {_("SLIDE_SPEED"), 12},
  {_("LOOP_START"), 13},
  {_("LOOP_END"), 14},
  {_("PATCH_END"), 0xff},
};

UPSFrame::UPSFrame(const wxString &title, const wxPoint &pos,
    const wxSize &size) :
  wxFrame(NULL, wxID_ANY, title, pos, size),
  valid_var_name("^[a-zA-Z\\_][a-zA-Z\\_0-9]*$") {
  wxMenu *menuFile = new wxMenu;
  menuFile->Append(wxID_NEW);
  menuFile->Append(wxID_OPEN);
  menuFile->Append(wxID_SAVE);
  menuFile->Append(wxID_SAVEAS);
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);
  wxMenu *menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);
  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(menuFile, _("&File"));
  menuBar->Append(menuHelp, _("&Help"));
  SetMenuBar(menuBar);

  wxToolBar *toolbar = CreateToolBar(wxTB_TEXT | wxTB_NOICONS);
  toolbar->AddTool(ID_PLAY, _("Play"), wxNullBitmap);
  toolbar->AddTool(ID_LOOP, _("Loop"), wxNullBitmap);
  toolbar->AddTool(ID_STOP, _("Stop"), wxNullBitmap);
  toolbar->AddTool(ID_STOP_ALL, _("Stop All"), wxNullBitmap);
  toolbar->AddTool(ID_SYNC, _("Sync Loops"), wxNullBitmap);
  toolbar->Realize();

  CreateStatusBar();

  data_tree = new wxTreeCtrl(this, ID_DATA_TREE, wxDefaultPosition,
      wxDefaultSize,
      wxTR_EDIT_LABELS | wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_HIDE_ROOT);
  data_tree_root = data_tree->AddRoot(_("Data Structures"));
  data_tree_patches = data_tree->AppendItem(data_tree_root,
      _("Sound Patches"));
  data_tree_structs = data_tree->AppendItem(data_tree_root,
      _("Patch Structs"));

  top_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
  right_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *data_control_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *data_control_sub_sizers[2] = {
    new wxBoxSizer(wxHORIZONTAL),
    new wxBoxSizer(wxHORIZONTAL),
  };
  wxBoxSizer *command_control_sizer = new wxBoxSizer(wxHORIZONTAL);

  data_control_sub_sizers[0]->Add(
      new wxButton(this, ID_NEW_PATCH, _("New Patch")));
  data_control_sub_sizers[0]->Add(
      new wxButton(this, ID_NEW_STRUCT, _("New Struct")));
  data_control_sub_sizers[1]->Add(
      new wxButton(this, ID_RENAME_DATA, _("Rename")));
  data_control_sub_sizers[1]->Add(
      new wxButton(this, ID_REMOVE_DATA, _("Remove")));
  data_control_sub_sizers[1]->Add(
      new wxButton(this, ID_CLONE_DATA, _("Clone")));

  data_control_sizer->Add(data_control_sub_sizers[0], 0, wxEXPAND);
  data_control_sizer->Add(data_control_sub_sizers[1], 0, wxEXPAND);

  left_sizer->Add(data_control_sizer, 0, wxEXPAND);
  left_sizer->Add(data_tree, wxEXPAND, wxEXPAND);

  command_control_sizer->Add(new wxBitmapButton(this, ID_NEW_COMMAND,
        wxArtProvider::GetBitmap(wxART_PLUS, wxART_BUTTON)));
  command_control_sizer->Add(new wxBitmapButton(this, ID_DELETE_COMMAND,
        wxArtProvider::GetBitmap(wxART_MINUS, wxART_BUTTON)));
  command_control_sizer->Add(new wxBitmapButton(this, ID_UP_COMMAND,
        wxArtProvider::GetBitmap(wxART_GO_UP, wxART_BUTTON)));
  command_control_sizer->Add(new wxBitmapButton(this, ID_DOWN_COMMAND,
        wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_BUTTON)));
  command_control_sizer->Add(new wxButton(this, ID_CLONE_COMMAND, _("Clone")));

  patch_grid = new UPSGrid(this, ID_PATCH_GRID);
  patch_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  patch_grid->SetColLabelValue(0, _("Delay"));
  patch_grid->SetColLabelValue(1, _("Command"));
  patch_grid->SetColLabelValue(2, _("Parameter"));
  patch_grid->SetColFormatNumber(0);
  patch_grid->SetColFormatNumber(2);
  patch_grid->DisableDragColSize();
  patch_grid->AutoSize();
  patch_grid->SetColSize(1, patch_grid->GetColSize(1)*2);
  patch_grid->DisableDragRowSize();
  patch_grid->EnableDragColMove();

  struct_grid = new UPSGrid(this, ID_STRUCT_GRID);
  struct_grid->CreateGrid(0, 5, wxGrid::wxGridSelectRows);
  struct_grid->SetColLabelValue(0, _("Type"));
  struct_grid->SetColLabelValue(1, _("PCM Data"));
  struct_grid->SetColLabelValue(2, _("Patch"));
  struct_grid->SetColLabelValue(3, _("Loop Start"));
  struct_grid->SetColLabelValue(4, _("Loop End"));
  struct_grid->DisableDragColSize();
  struct_grid->AutoSize();
  struct_grid->SetColSize(0, struct_grid->GetColSize(0)*2);
  struct_grid->SetColSize(2, struct_grid->GetColSize(2)*3);
  struct_grid->DisableDragRowSize();
  struct_grid->EnableDragColMove();

  right_sizer->Add(command_control_sizer, 0, wxEXPAND);
  right_sizer->Add(patch_grid, wxEXPAND, wxEXPAND);
  right_sizer->Add(struct_grid, wxEXPAND, wxEXPAND);

  top_sizer->Add(left_sizer, wxEXPAND, wxEXPAND);
  top_sizer->Add(right_sizer, wxEXPAND, wxEXPAND);

  top_sizer->Hide(1);

  SetSizer(top_sizer);
  update_layout();

  wxAcceleratorEntry accelerator_entries[] = {
    wxAcceleratorEntry(wxACCEL_CTRL, WXK_RETURN, ID_PLAY),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'L', ID_LOOP),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'A', ID_STOP_ALL),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'T', ID_STOP),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'Y', ID_SYNC),
    wxAcceleratorEntry(wxACCEL_CTRL, WXK_UP, ID_UP_COMMAND),
    wxAcceleratorEntry(wxACCEL_CTRL, WXK_DOWN, ID_DOWN_COMMAND),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'C', ID_CLONE_COMMAND),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'D', ID_DELETE_COMMAND),
    wxAcceleratorEntry(wxACCEL_CTRL, (int) 'E', ID_NEW_COMMAND),
  };
  SetAcceleratorTable(wxAcceleratorTable(
        sizeof(accelerator_entries)/sizeof(wxAcceleratorEntry),
        accelerator_entries));
}

void UPSFrame::on_exit(wxCommandEvent &event) {
  (void) event;
  Close(true);
}

void UPSFrame::on_about(wxCommandEvent &event) {
  (void) event;
  wxAboutDialogInfo info;
  info.SetName(_("Uzebox Patch Studio"));
  info.SetVersion(_("0.0.1"));
  info.SetDescription(_("A graphical sound patch editor for the Uzebox"));
  info.SetCopyright(_("(c) 2016"));
  info.AddDeveloper(_("Flavio Zavan"));
  wxAboutBox(info);
}

void UPSFrame::on_new(wxCommandEvent &event) {
  (void) event;

  clear();
}

void UPSFrame::on_data_tree_label_edit(wxTreeEvent &event) {
  if (event.GetItem() == data_tree_patches
      || event.GetItem() == data_tree_structs)
    event.Veto();
}

void UPSFrame::on_data_tree_changed(wxTreeEvent &event) {
  auto item = event.GetItem();
  auto old_item = event.GetOldItem();

  patch_grid->EnableEditing(false);
  struct_grid->EnableEditing(false);
  if (old_item.IsOk()
      && data_tree->GetItemParent(old_item) == data_tree_patches) {
    update_patch_data(old_item);
  }
  else if (old_item.IsOk()
      && data_tree->GetItemParent(old_item) == data_tree_structs) {
    update_struct_data(old_item);
  }

  if (item.IsOk()) {
    auto parent = data_tree->GetItemParent(item);
    if (parent == data_tree_patches) {
      read_patch_data(item);
      top_sizer->Show(1, true);
      right_sizer->Show(1, true);
      right_sizer->Show(2, false);
    }
    else if (parent == data_tree_structs) {
      read_struct_data(item);
      top_sizer->Show(1, true);
      right_sizer->Show(1, false);
      right_sizer->Show(2, true);
    }
    else {
      top_sizer->Show(1, false);
    }

    update_layout();
  }
  patch_grid->EnableEditing(true);
  struct_grid->EnableEditing(true);
}

void UPSFrame::on_new_patch(wxCommandEvent &event) {
  (void) event;
  wxString name = get_next_data_name(wxT("patch"));
  wxTreeItemId c = data_tree->AppendItem(data_tree_patches, name);
  patch_names.insert(name);
  data_tree->SetItemData(c, new PatchData());
  data_tree->SelectItem(c);
  data_tree->EditLabel(c);
}

void UPSFrame::on_new_struct(wxCommandEvent &event) {
  (void) event;
  wxString name = get_next_data_name(wxT("patchstruct"));
  wxTreeItemId c = data_tree->AppendItem(data_tree_structs, name);
  data_tree->SetItemData(c, new StructData());
  data_tree->SelectItem(c);
  data_tree->EditLabel(c);
}

void UPSFrame::on_rename(wxCommandEvent &event) {
  (void) event;
  data_tree->EditLabel(data_tree->GetSelection());
}

wxTreeItemId UPSFrame::find_data(const wxTreeItemId &root,
    const wxString &name) {
  wxTreeItemIdValue cookie;
  auto item = data_tree->GetFirstChild(root, cookie);

  while (item.IsOk()) {
    if (data_tree->GetItemText(item) == name)
      return item;

    wxTreeItemId child = find_data(item, name);
    if (child.IsOk())
      return child;

    item = data_tree->GetNextChild(root, cookie);
  }

  return wxTreeItemId();
}

wxString UPSFrame::get_next_data_name(const wxString &base, bool try_bare) {
  wxTreeItemId item;
  wxString next;

  if (try_bare) {
    item = find_data(data_tree_root, base);
    if (!item.IsOk())
      return base;
  }

  int i = 0;
  do {
    next = wxString::Format(wxT("%s%02d"), base, i++);
    item = find_data(data_tree_root, next);
  } while (item.IsOk());

  return next;
}

void UPSFrame::on_data_tree_label_edit_end(wxTreeEvent &event) {
  auto label = event.GetLabel();

  if(!validate_var_name(label)) {
    SetStatusText(_("Invalid variable name"));
    event.Veto();
    return;
  }
  else if (find_data(data_tree_root, label).IsOk()) {
    SetStatusText(_("Name already in use"));
    event.Veto();
    return;
  }

  auto item = event.GetItem();
  if (data_tree->GetItemParent(item) == data_tree_patches) {
    auto old_label = data_tree->GetItemText(item);

    patch_names.erase(old_label);
    patch_names.insert(label);

    replace_patch_in_structs(old_label, label);
  }
}

bool UPSFrame::validate_var_name(const wxString &name) {
  return name != wxT("NULL") && valid_var_name.Matches(name);
}

int UPSFrame::add_patch_command(const wxString &delay, const wxString &command,
    const wxString &param, int pos) {
  int row_num;
  if (pos == -1) {
    row_num = patch_grid->GetNumberRows();
    patch_grid->AppendRows();
  }
  else {
    row_num = pos;
    patch_grid->InsertRows(pos);
  }
  patch_grid->SetCellEditor(row_num, 1, new wxGridCellChoiceEditor(16,
        command_choices, false));
  patch_grid->SetCellValue(delay, row_num, 0);
  patch_grid->SetCellValue(command, row_num, 1);
  patch_grid->SetCellValue(param, row_num, 2);

  update_patch_row_colors(row_num);
  patch_grid->deselect_cells();

  return row_num;
}

void UPSFrame::on_new_command(wxCommandEvent &event) {
  (void) event;
  /* Patch grid is at position 1 */
  if (right_sizer->IsShown(1)) {
    int row_num = add_patch_command();
    patch_grid->GoToCell(row_num, 1);
  }
  else if (right_sizer->IsShown(2)) {
    int row_num = add_struct_command();
    struct_grid->GoToCell(row_num, 1);
  }
}

void UPSFrame::on_delete_command(wxCommandEvent &event) {
  (void) event;
  if (!right_sizer->IsShown(1) && !right_sizer->IsShown(2)) {
    return ;
  }

  auto grid = right_sizer->IsShown(1)? patch_grid : struct_grid;
  wxArrayInt selected = grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*b - *a); });
  for (auto row : selected)
    grid->DeleteRows(row);
}

void UPSFrame::on_up_command(wxCommandEvent &event) {
  (void) event;
  if (!right_sizer->IsShown(1) && !right_sizer->IsShown(2)) {
    return ;
  }

  auto grid = right_sizer->IsShown(1)? patch_grid : struct_grid;
  wxArrayInt selected = grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*a - *b); });

  /* This forces the cell that is being edited to update its value */
  grid->EnableEditing(false);
  grid->EnableEditing(true);

  wxArrayString v;
  v.Add(wxEmptyString, grid->GetNumberCols());
  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    for (size_t j = 0; j < v.GetCount(); j++) {
      v[j] = grid->GetCellValue(row, j);
    }

    grid->DeleteRows(row);
    row = std::max(i, row-1);
    if (grid == patch_grid) {
      add_patch_command(v[0], v[1], v[2], row);
    }
    else {
      add_struct_command(v[0], v[1], v[2], v[3], v[4], row);
    }
    grid->SelectRow(row, true);
  }
}


void UPSFrame::on_down_command(wxCommandEvent &event) {
  (void) event;
  if (!right_sizer->IsShown(1) && !right_sizer->IsShown(2)) {
    return ;
  }

  auto grid = right_sizer->IsShown(1)? patch_grid : struct_grid;
  wxArrayInt selected = grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*b - *a); });

  /* This forces the cell that is being edited to update its value */
  grid->EnableEditing(false);
  grid->EnableEditing(true);

  int last_row = grid->GetNumberRows()-1;
  wxArrayString v;
  v.Add(wxEmptyString, grid->GetNumberCols());
  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    for (size_t j = 0; j < v.GetCount(); j++) {
      v[j] = grid->GetCellValue(row, j);
    }

    grid->DeleteRows(row);
    row = std::min(last_row-i, row+1);
    if (grid == patch_grid) {
      add_patch_command(v[0], v[1], v[2], row);
    }
    else {
      add_struct_command(v[0], v[1], v[2], v[3], v[4], row);
    }
    grid->SelectRow(row, true);
  }
}

void UPSFrame::on_clone_command(wxCommandEvent &event) {
  (void) event;
  if (!right_sizer->IsShown(1) && !right_sizer->IsShown(2)) {
    return ;
  }

  auto grid = right_sizer->IsShown(1)? patch_grid : struct_grid;
  wxArrayInt selected = grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*a - *b); });

  /* This forces the cell that is being edited to update its value */
  grid->EnableEditing(false);
  grid->EnableEditing(true);

  wxArrayString v;
  v.Add(wxEmptyString, grid->GetNumberCols());
  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    for (size_t j = 0; j < v.GetCount(); j++) {
      v[j] = grid->GetCellValue(row, j);
    }

    if (grid == patch_grid) {
      add_patch_command(v[0], v[1], v[2]);
    }
    else {
      add_struct_command(v[0], v[1], v[2], v[3], v[4]);
    }
  }
}

void UPSFrame::on_cell_changed(wxGridEvent &event) {
  /* Patch grid is at position 1 */
  if (right_sizer->IsShown(1)) {
    update_patch_row_colors(event.GetRow());
  }
  else if (right_sizer->IsShown(2)) {
    update_struct_row_colors(event.GetRow());
  }
}

void UPSFrame::update_patch_data(const wxTreeItemId &item) {
  auto data = (PatchData *) data_tree->GetItemData(item);
  data->data.clear();

  for (int row = 0; row < patch_grid->GetNumberRows(); row++) {
    long delay, command, param;
    patch_grid->GetCellValue(row, 0).ToLong(&delay);
    command = command_ids.find(patch_grid->GetCellValue(row, 1))->second;
    patch_grid->GetCellValue(row, 2).ToLong(&param);

    data->data.push_back(delay);
    data->data.push_back(command);
    data->data.push_back(param);
  }
}

void UPSFrame::read_patch_data(const wxTreeItemId &item) {
  auto data = (PatchData *) data_tree->GetItemData(item);

  if (patch_grid->GetNumberRows()) {
    patch_grid->DeleteRows(0, patch_grid->GetNumberRows());
  }

  for (size_t i = 0; i < data->data.size(); i += 3) {
    add_patch_command(wxString::Format(wxT("%ld"), data->data[i]),
        command_choices[std::min(15l, data->data[i+1])],
        wxString::Format(wxT("%ld"), data->data[i+2]));
  }
}

void UPSFrame::update_patch_row_colors(int row) {
  long param, delay;
  patch_grid->GetCellValue(row, 0).ToLong(&delay);
  patch_grid->GetCellValue(row, 2).ToLong(&param);

  /* Delay color, always an unsigned integer */
  patch_grid->SetCellBackgroundColour(row, 0,
      delay < 0 || delay > 255? wxColor(127, 0, 0) : wxColour(0, 127, 0));

  /* Command color is always green */
  patch_grid->SetCellBackgroundColour(row, 1, wxColour(0, 127, 0));

  /* Param limit depends on the command */
  auto limit = limits.find(patch_grid->GetCellValue(row, 1));
  patch_grid->SetCellBackgroundColour(row, 2,
      param < limit->second.first || param > limit->second.second?
      wxColor(127, 0, 0) : wxColour(0, 127, 0));
}

void UPSFrame::on_save(wxCommandEvent &event) {
  (void) event;

  if (current_file_path.IsEmpty()) {
    on_save_as(event);
    return;
  }

  save_to_file(current_file_path);
}

void UPSFrame::on_save_as(wxCommandEvent &event) {
  (void) event;

  wxFileDialog file_dialog(this, _("Save"), wxEmptyString,
      current_file_path.IsEmpty()? _("patches.inc") : current_file_path,
      wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (file_dialog.ShowModal() == wxID_CANCEL) {
    return;
  }

  save_to_file(file_dialog.GetPath());
}

void UPSFrame::save_to_file(const wxString &path) {
  wxTextFile file(path);
  file.Create();
  file.Open();
  if (!file.IsOpened()) {
    SetStatusText(wxString::Format(_("Failed to write to %s"), path));
    return;
  }

  /* This forces the cell that is being edited to update its value */
  patch_grid->EnableEditing(false);
  patch_grid->EnableEditing(true);
  struct_grid->EnableEditing(false);
  struct_grid->EnableEditing(true);

  file.Clear();

  /* Patches have to come first */
  wxTreeItemIdValue cookie;
  auto item = data_tree->GetFirstChild(data_tree_patches, cookie);
  while (item.IsOk()) {
    if (data_tree->IsSelected(item))
      update_patch_data(item);

    file.AddLine(wxString::Format("const char %s[] PROGMEM = {",
          data_tree->GetItemText(item)));

    auto data = (PatchData *) data_tree->GetItemData(item);
    if (data->data.empty()) {
      file.AddLine(wxT("  0, PC_PATCH_END, 0,"));
    }
    for (size_t i = 0; i < data->data.size(); i += 3)
      file.AddLine(wxString::Format("  %ld, PC_%s, %ld,",
            data->data[i], command_choices[std::min(15l, data->data[i+1])],
            data->data[i+2]));

    file.AddLine(wxT("};"));

    item = data_tree->GetNextChild(data_tree_patches, cookie);
  }

  item = data_tree->GetFirstChild(data_tree_structs, cookie);
  while (item.IsOk()) {
    if (data_tree->IsSelected(item))
      update_struct_data(item);

    file.AddLine(wxString::Format("const struct PatchStruct %s[] PROGMEM = {",
          data_tree->GetItemText(item)));

    auto data = (StructData *) data_tree->GetItemData(item);
    if (data->data.empty()) {
      file.AddLine(wxT("  {0, NULL, NULL, 0, 0},"));
    }
    for (size_t i = 0; i < data->data.size(); i += 5)
      file.AddLine(wxString::Format("  {%ld, %s, %s, %s, %s},",
            choice_values.find(data->data[i])->second, data->data[i+1],
            data->data[i+2], data->data[i+3], data->data[i+4]));

    file.AddLine(wxT("};"));

    item = data_tree->GetNextChild(data_tree_structs, cookie);
  }

  file.Write(wxTextFileType_Unix);

  SetStatusText(wxString::Format(_("%s written"), path));
  current_file_path = path;

  SetTitle(wxString::Format(_("Uzebox Patch Studio - %s"), current_file_path));
}

void UPSFrame::on_open(wxCommandEvent &event) {
  (void) event;

  wxFileDialog file_dialog(this, _("Open"), wxEmptyString, wxEmptyString,
      wxFileSelectorDefaultWildcardStr,
      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);

  if (file_dialog.ShowModal() == wxID_CANCEL)
    return;

  open_file(file_dialog.GetPath());
}

void UPSFrame::open_file(const wxString &path) {
  std::multimap<wxString, wxVector<long>> patches;
  std::multimap<wxString, wxVector<wxString>> structs;
  if (!FileReader::read_patches_and_structs(path, patches, structs)) {
    SetStatusText(wxString::Format(_("Failed to open %s"), path));
    return;
  }

  /* Clean the data */
  clear();

  /* Add all the patches */
  for (auto &p : patches) {
    wxString name = get_next_data_name(p.first, true);
    wxTreeItemId c = data_tree->AppendItem(data_tree_patches, name);
    patch_names.insert(name);

    PatchData *data = new PatchData();

    for (size_t i = 0; i < p.second.size(); i += 3) {
      /* Delay */
      data->data.push_back(p.second[i]);
      /* Command */
      data->data.push_back(std::min(15l, (long) p.second[i+1]));
      /* Parameter. PATCH_END might not have one */
      data->data.push_back(data->data.back() == 15 && i+2 >= p.second.size()?
          0 : p.second[i+2]);
    }

    data_tree->SetItemData(c, data);
  }
  /* Add all the structs */
  for (auto &s : structs) {
    wxString name = get_next_data_name(s.first, true);
    wxTreeItemId c = data_tree->AppendItem(data_tree_structs, name);

    StructData *data = new StructData();

    for (size_t i = 0; i < s.second.size(); i += 5) {
      long type = strtol(s.second[i].c_str(), NULL, 0);
      type = std::min(2l, std::max(0l, type));
      data->data.push_back(type_choices[type]);

      for (size_t j = 1; j < 5; j++) {
        data->data.push_back(s.second[i+j]);
      }
    }

    data_tree->SetItemData(c, data);
  }

  SetStatusText(wxString::Format(
        _("%s opened with %lu patches and %lu structs"),
        path, patches.size(), structs.size()));

  data_tree->ExpandAll();
  current_file_path = path;

  SetTitle(wxString::Format(_("Uzebox Patch Studio - %s"), current_file_path));
}

void UPSFrame::clear() {
  data_tree->DeleteChildren(data_tree_patches);
  data_tree->DeleteChildren(data_tree_structs);

  top_sizer->Hide(1);
  update_layout();
}

void UPSFrame::on_play(wxCommandEvent &event) {
  (void) event;

  auto item = data_tree->GetSelection();
  if (item.IsOk() && data_tree->GetItemParent(item) == data_tree_patches) {
    /* Force updates */
    patch_grid->EnableEditing(false);
    patch_grid->EnableEditing(true);
    update_patch_data(item);

    auto data = (PatchData *) data_tree->GetItemData(item);
    if (data->play()) {
      SetStatusText(wxString::Format(_("Playing %s"),
            data_tree->GetItemText(item)));
    }
    else {
      SetStatusText(data->last_error);
    }
  }
}

void UPSFrame::on_loop(wxCommandEvent &event) {
  (void) event;

  auto item = data_tree->GetSelection();
  if (item.IsOk() && data_tree->GetItemParent(item) == data_tree_patches) {
    /* Force updates */
    patch_grid->EnableEditing(false);
    patch_grid->EnableEditing(true);
    update_patch_data(item);

    auto data = (PatchData *) data_tree->GetItemData(item);
    if (data->play(true)) {
      SetStatusText(wxString::Format(_("Looping %s"),
            data_tree->GetItemText(item)));
      data_tree->SetItemBold(item);
    }
    else {
      SetStatusText(wxString::Format(_("Failed to loop %s"),
            data_tree->GetItemText(item)));
    }
  }
}

void UPSFrame::on_stop(wxCommandEvent &event) {
  (void) event;

  auto item = data_tree->GetSelection();
  if (item.IsOk() && data_tree->GetItemParent(item) == data_tree_patches) {
    auto data = (PatchData *) data_tree->GetItemData(item);
    data->stop();
    data_tree->SetItemBold(item, false);
  }
}

void UPSFrame::on_stop_all(wxCommandEvent &event) {
  (void) event;

  wxTreeItemIdValue cookie;
  auto item = data_tree->GetFirstChild(data_tree_patches, cookie);
  while (item.IsOk()) {
    auto data = (PatchData *) data_tree->GetItemData(item);
    data->stop();
    data_tree->SetItemBold(item, false);
    item = data_tree->GetNextChild(data_tree_patches, cookie);
  }
}

int UPSFrame::add_struct_command(const wxString &type, const wxString &pcm,
    const wxString &patch, const wxString &loop_start,
    const wxString &loop_end, int pos) {
  int row_num;
  if (pos == -1) {
    row_num = struct_grid->GetNumberRows();
    struct_grid->AppendRows();
  }
  else {
    row_num = pos;
    struct_grid->InsertRows(pos);
  }

  struct_grid->SetCellEditor(row_num, 0, new wxGridCellChoiceEditor(3,
        type_choices, false));
  struct_grid->SetCellEditor(row_num, 2, new wxGridCellChoiceEditor(
        patch_names.size(),
        &(wxVector<wxString>(patch_names.begin(), patch_names.end()))[0],
        true));
  struct_grid->SetCellValue(type, row_num, 0);
  struct_grid->SetCellValue(pcm, row_num, 1);
  struct_grid->SetCellValue(patch == wxEmptyString && patch_names.size()?
      *(patch_names.begin()) : patch, row_num, 2);
  struct_grid->SetCellValue(loop_start, row_num, 3);
  struct_grid->SetCellValue(loop_end, row_num, 4);

  update_struct_row_colors(row_num);
  struct_grid->deselect_cells();

  return row_num;
}

void UPSFrame::update_struct_row_colors(int row) {
  long loop_start = strtol(struct_grid->GetCellValue(row, 3), NULL, 0);
  long loop_end = strtol(struct_grid->GetCellValue(row, 4), NULL, 0);

  /* Type color is always green */
  struct_grid->SetCellBackgroundColour(row, 0, wxColour(0, 127, 0));

  auto pcm_data = struct_grid->GetCellValue(row, 1);
  bool type_is_pcm = struct_grid->GetCellValue(row, 0) == _("PCM");
  if (type_is_pcm) {
    struct_grid->SetCellBackgroundColour(row, 1,
        pcm_data == wxT("NULL")? wxColor(127, 0, 0) : wxColour(0, 127, 0));
  }
  else {
    struct_grid->SetCellBackgroundColour(row, 1,
        pcm_data != wxT("NULL")?  wxColor(127, 0, 0) : wxColour(0, 127, 0));
  }

  auto patch = struct_grid->GetCellValue(row, 2);
  if (type_is_pcm) {
    struct_grid->SetCellBackgroundColour(row, 2,
        patch != wxT("NULL")? wxColor(127, 0, 0) : wxColour(0, 127, 0));
  }
  else {
    struct_grid->SetCellBackgroundColour(row, 2,
        patch == wxT("NULL") || patch_names.find(patch) == patch_names.end()?
        wxColor(127, 0, 0) : wxColour(0, 127, 0));
  }

  /* 16 bit unsigned integers or whatever string */
  struct_grid->SetCellBackgroundColour(row, 3,
      loop_start < 0 || loop_start >= 1<<16?
      wxColor(127, 0, 0) : wxColour(0, 127, 0));
  struct_grid->SetCellBackgroundColour(row, 4,
      loop_end < 0 || loop_end >= 1<<16?
      wxColor(127, 0, 0) : wxColour(0, 127, 0));
}

void UPSFrame::update_struct_data(const wxTreeItemId &item) {
  auto data = (StructData *) data_tree->GetItemData(item);
  data->data.clear();

  for (int row = 0; row < struct_grid->GetNumberRows(); row++) {
    for (int col = 0; col < 5; col++) {
      data->data.push_back(struct_grid->GetCellValue(row, col));
    }
  }
}

void UPSFrame::read_struct_data(const wxTreeItemId &item) {
  auto data = (StructData *) data_tree->GetItemData(item);

  if (struct_grid->GetNumberRows()) {
    struct_grid->DeleteRows(0, struct_grid->GetNumberRows());
  }

  for (size_t i = 0; i < data->data.size(); i += 5) {
    add_struct_command(data->data[i], data->data[i+1],
        data->data[i+2], data->data[i+3], data->data[i+4]);
  }
}

void UPSFrame::replace_patch_in_structs(const wxString &src,
    const wxString &dst) {
  wxTreeItemIdValue cookie;
  auto item = data_tree->GetFirstChild(data_tree_structs, cookie);
  while (item.IsOk()) {
    auto data = (StructData *) data_tree->GetItemData(item);
    for (size_t i = 0; i < data->data.size(); i += 5) {
      if (data->data[i+2] == src) {
        data->data[i+2] = dst;
      }
    }
    item = data_tree->GetNextChild(data_tree_structs, cookie);
  }
}

void UPSFrame::on_remove(wxCommandEvent &event) {
  (void) event;
  auto item = data_tree->GetSelection();
  if (!item.IsOk()) {
    return;
  }

  auto parent = data_tree->GetItemParent(item);

  if (parent != data_tree_root) {
    data_tree->Delete(item);
  }
}

void UPSFrame::on_clone_data(wxCommandEvent &event) {
  (void) event;
  auto item = data_tree->GetSelection();
  if (!item.IsOk()) {
    return;
  }

  auto parent = data_tree->GetItemParent(item);
  if (parent == data_tree_root) {
    return;
  }

  auto name = get_next_data_name(data_tree->GetItemText(item));
  auto c = data_tree->AppendItem(parent, name);
  if (parent == data_tree_patches) {
    data_tree->SetItemData(c,
        new PatchData((PatchData *) data_tree->GetItemData(item)));
  }
  else if (parent == data_tree_structs) {
    data_tree->SetItemData(c,
        new StructData((StructData *) data_tree->GetItemData(item)));
  }
}

void UPSFrame::on_sync(wxCommandEvent &event) {
  (void) event;

  wxTreeItemIdValue cookie;
  auto item = data_tree->GetFirstChild(data_tree_patches, cookie);
  while (item.IsOk()) {
    auto data = (PatchData *) data_tree->GetItemData(item);
    data->retrigger();
    item = data_tree->GetNextChild(data_tree_patches, cookie);
  }
}

void UPSFrame::update_layout() {
  top_sizer->Layout();
  auto min_size = top_sizer->GetMinSize();
  min_size.SetHeight(MIN_CLIENT_HEIGHT);
  SetMinClientSize(min_size);
}
