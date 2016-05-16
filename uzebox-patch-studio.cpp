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
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "grid.h"
#include "input.h"
#include "generate.h"

class PatchData : public wxTreeItemData {
  public:
    wxVector<long> data;
};

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
    void on_patch_cell_changed(wxGridEvent &event);
    void on_save_as(wxCommandEvent &event);
    void on_save(wxCommandEvent &event);
    void on_open(wxCommandEvent &event);
    void on_play(wxCommandEvent &event);

    bool validate_var_name(const wxString &name);

    wxString get_next_data_name(const wxString &base, bool try_bare=false);
    wxTreeItemId find_data(const wxTreeItemId &item, const wxString &name);
    int add_patch_command(const wxString &delay="0",
        const wxString &command=command_choices[0],
        const wxString &param="0",
        int pos=-1);
    void update_patch_data(const wxTreeItemId &item);
    void read_patch_data(const wxTreeItemId &item);
    void update_patch_row_colors(int row);
    void save_to_file(const wxString &path);
    void clear();

    wxRegEx valid_var_name;
    wxTreeItemId data_tree_root;
    wxTreeItemId data_tree_patches;
    wxTreeItemId data_tree_structs;
    wxTreeCtrl *data_tree;
    wxGrid *patch_grid;
    wxBoxSizer *top_sizer;
    wxString current_file_path;
    wxVector<uint8_t> current_wave_data;
    Mix_Chunk *current_wave = nullptr;

    static const std::map<wxString, std::pair<long, long>> limits;
    static const wxString command_choices[16];
    static const std::map<wxString, long> command_ids;

    wxDECLARE_EVENT_TABLE();
};

enum {
  ID_PLAY = 1,
  ID_LOOP,
  ID_STOP,
  ID_DATA_TREE,
  ID_NEW_PATCH,
  ID_NEW_STRUCT,
  ID_RENAME_DATA,
  ID_REMOVE_DATA,
  ID_PATCH_GRID,
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
  EVT_GRID_CELL_CHANGED(UPSFrame::on_patch_cell_changed)
  EVT_MENU(wxID_SAVEAS, UPSFrame::on_save_as)
  EVT_MENU(wxID_SAVE, UPSFrame::on_save)
  EVT_MENU(wxID_OPEN, UPSFrame::on_open)
  EVT_MENU(ID_PLAY, UPSFrame::on_play)
wxEND_EVENT_TABLE()
wxIMPLEMENT_APP(UPSApp);

bool UPSApp::OnInit() {
  if (SDL_Init(SDL_INIT_AUDIO) == -1
      || Mix_OpenAudio(SAMPLE_RATE, AUDIO_U8, 1, 4096) == -1) {
    return false;
  }

  UPSFrame *frame = new UPSFrame(_("Uzebox Patch Studio"),
      wxPoint(50, 50), wxSize(600, 400));
  frame->Show(true);

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
  {_("WAVE"), std::make_pair(0, 9)},
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
  toolbar->Realize();

  CreateStatusBar();
  SetStatusText(_("Uzebox Patch Studio"));

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
  wxBoxSizer *right_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *data_control_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *command_control_sizer = new wxBoxSizer(wxHORIZONTAL);

  data_control_sizer->Add(new wxButton(this, ID_NEW_PATCH, _("New Patch")));
  data_control_sizer->Add(new wxButton(this, ID_NEW_STRUCT, _("New Struct")));
  data_control_sizer->Add(new wxButton(this, ID_RENAME_DATA, _("Rename")));
  data_control_sizer->Add(new wxButton(this, ID_REMOVE_DATA, _("Remove")));

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

  right_sizer->Add(command_control_sizer, 0, wxEXPAND);
  right_sizer->Add(patch_grid, wxEXPAND, wxEXPAND);

  top_sizer->Add(left_sizer, wxEXPAND, wxEXPAND);
  top_sizer->Add(right_sizer, wxEXPAND, wxEXPAND);

  top_sizer->Hide(1);

  SetSizerAndFit(top_sizer);
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
  if (old_item.IsOk()
      && data_tree->GetItemParent(old_item) == data_tree_patches) {
    update_patch_data(old_item);
  }
  else if (old_item.IsOk()
      && data_tree->GetItemParent(old_item) == data_tree_structs) {
  }

  if (item.IsOk() && data_tree->GetItemParent(item) == data_tree_patches) {
    read_patch_data(item);
    top_sizer->Show(1, true);
  }
  else if (data_tree->GetItemParent(item) == data_tree_structs) {
  }
  SetSizerAndFit(top_sizer);
  patch_grid->EnableEditing(true);
}

void UPSFrame::on_new_patch(wxCommandEvent &event) {
  (void) event;
  wxString name = get_next_data_name(wxT("patch"));
  wxTreeItemId c = data_tree->AppendItem(data_tree_patches, name);
  data_tree->SetItemData(c, new PatchData());
  data_tree->SelectItem(c);
  data_tree->EditLabel(c);
}

void UPSFrame::on_new_struct(wxCommandEvent &event) {
  (void) event;
  wxString name = get_next_data_name(wxT("patchstruct"));
  wxTreeItemId c = data_tree->AppendItem(data_tree_structs, name);
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

  if (find_data(data_tree_root, label).IsOk()
      || !validate_var_name(label))
    event.Veto();
}

bool UPSFrame::validate_var_name(const wxString &name) {
  return valid_var_name.Matches(name);
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
  patch_grid->SetCellBackgroundColour(row_num, 0, wxColour(0, 127, 0));
  patch_grid->SetCellBackgroundColour(row_num, 1, wxColour(0, 127, 0));
  patch_grid->SetCellBackgroundColour(row_num, 2, wxColour(0, 127, 0));

  update_patch_row_colors(row_num);

  return row_num;
}

void UPSFrame::on_new_command(wxCommandEvent &event) {
  (void) event;
  int row_num = add_patch_command();
  patch_grid->GoToCell(row_num, 1);
}

void UPSFrame::on_delete_command(wxCommandEvent &event) {
  (void) event;
  wxArrayInt selected = patch_grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*b - *a); });
  for (auto row : selected)
    patch_grid->DeleteRows(row);
}

void UPSFrame::on_up_command(wxCommandEvent &event) {
  (void) event;
  wxArrayInt selected = patch_grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*a - *b); });

  /* This forces the cell that is being edited to update its value */
  patch_grid->EnableEditing(false);
  patch_grid->EnableEditing(true);

  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    wxString delay = patch_grid->GetCellValue(row, 0);
    wxString command = patch_grid->GetCellValue(row, 1);
    wxString param = patch_grid->GetCellValue(row, 2);

    patch_grid->DeleteRows(row);
    row = std::max(i, row-1);
    add_patch_command(delay, command, param, row);
    patch_grid->SelectRow(row, true);
  }
}


void UPSFrame::on_down_command(wxCommandEvent &event) {
  (void) event;
  wxArrayInt selected = patch_grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*b - *a); });

  /* This forces the cell that is being edited to update its value */
  patch_grid->EnableEditing(false);
  patch_grid->EnableEditing(true);

  int last_row = patch_grid->GetNumberRows()-1;
  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    wxString delay = patch_grid->GetCellValue(row, 0);
    wxString command = patch_grid->GetCellValue(row, 1);
    wxString param = patch_grid->GetCellValue(row, 2);

    patch_grid->DeleteRows(row);
    row = std::min(last_row-i, row+1);
    add_patch_command(delay, command, param, row);
    patch_grid->SelectRow(row, true);
  }
}

void UPSFrame::on_clone_command(wxCommandEvent &event) {
  (void) event;
  wxArrayInt selected = patch_grid->GetSelectedRows();
  selected.Sort([] (int *a, int *b) { return (*a - *b); });

  /* This forces the cell that is being edited to update its value */
  patch_grid->EnableEditing(false);
  patch_grid->EnableEditing(true);

  for (int i = 0; i < (int) selected.GetCount(); i++) {
    int row = selected[i];

    wxString delay = patch_grid->GetCellValue(row, 0);
    wxString command = patch_grid->GetCellValue(row, 1);
    wxString param = patch_grid->GetCellValue(row, 2);
    add_patch_command(delay, command, param);
  }
}

void UPSFrame::on_patch_cell_changed(wxGridEvent &event) {
  update_patch_row_colors(event.GetRow());
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

  if (patch_grid->GetNumberRows())
    patch_grid->DeleteRows(0, patch_grid->GetNumberRows());

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

  wxFileDialog file_dialog(this, _("Save"), wxEmptyString, _("patches.inc"),
      wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (file_dialog.ShowModal() == wxID_CANCEL)
    return;

  save_to_file(file_dialog.GetPath());
}

void UPSFrame::save_to_file(const wxString &path) {
  wxTextFile file(path);
  if (!file.Create())
    file.Open();
  if (!file.IsOpened())
    return;

  /* This forces the cell that is being edited to update its value */
  patch_grid->EnableEditing(false);
  patch_grid->EnableEditing(true);

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
    if (data->data.empty())
      file.AddLine(wxT("  0, PC_PATCH_END, 0,"));
    for (size_t i = 0; i < data->data.size(); i += 3)
      file.AddLine(wxString::Format("  %ld, PC_%s, %ld,",
            data->data[i], command_choices[std::min(15l, data->data[i+1])],
            data->data[i+2]));

    file.AddLine(wxT("}"));

    item = data_tree->GetNextChild(data_tree_patches, cookie);
  }

  file.Write(wxTextFileType_Unix);

  SetStatusText(wxString::Format(_("%s written"), path));
  current_file_path = path;
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
  std::map<wxString, wxVector<long>> patches;
  if (!read_patches(path, patches)) {
    SetStatusText(wxString::Format(_("Failed to open %s"), path));
    return;
  }

  /* Clean the data */
  clear();

  /* Add all the patches */
  for (auto &p : patches) {
    wxString name = get_next_data_name(p.first, true);
    wxTreeItemId c = data_tree->AppendItem(data_tree_patches, name);

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

  SetStatusText(wxString::Format(_("%s opened with %lu patches"),
        path, patches.size()));

  data_tree->ExpandAll();
}

void UPSFrame::clear() {
  data_tree->DeleteChildren(data_tree_patches);
  data_tree->DeleteChildren(data_tree_structs);

  top_sizer->Hide(1);
  SetSizerAndFit(top_sizer);
}

void UPSFrame::on_play(wxCommandEvent &event) {
  (void) event;

  auto item = data_tree->GetSelection();
  if (item.IsOk() && data_tree->GetItemParent(item) == data_tree_patches) {
    /* Force updates */
    patch_grid->EnableEditing(false);
    patch_grid->EnableEditing(true);

    wxLog::AddTraceMask("sound");
    auto data = (PatchData *) data_tree->GetItemData(item);
    if (current_wave != nullptr) {
      Mix_FreeChunk(current_wave);
      current_wave = nullptr;
    }
    if (generate_wave(data->data, current_wave_data)
        && (current_wave = Mix_LoadWAV_RW(SDL_RWFromMem(
              &(current_wave_data[0]), current_wave_data.size()), 0))) {
      Mix_PlayChannel(-1, current_wave, 0);

      SetStatusText(wxString::Format(_("Playing %s"),
            data_tree->GetItemText(item)));
    }
    else {
      SetStatusText(wxString::Format(_("Failed to play %s"),
            data_tree->GetItemText(item)));
    }
  }
}
