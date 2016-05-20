class StructData : public wxTreeItemData {
  public:
    StructData();
    StructData(const StructData *p);

    wxVector<wxString> data;
};
