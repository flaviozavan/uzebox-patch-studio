class FileReader {
  public:
    static bool read_patches_and_structs(const wxString &fn,
        std::multimap<wxString, wxVector<long>> &patches,
        std::multimap<wxString, wxVector<wxString>> &structs);

  private:
    static long string_to_long(const wxString &str);
    static bool read_patch_vals(const wxString &str, wxVector<long> &vals);
    static std::string clean_code(const std::string &code);
    static bool read_struct_vals(const wxString &str,
        wxVector<wxString> &vals);
    static bool read_patches(const std::string &clean_src,
        std::multimap<wxString, wxVector<long>> &data);
    static bool read_structs(const std::string &clean_src,
        std::multimap<wxString, wxVector<wxString>> &data);

    static const std::map<wxString, long> defines;
    static const std::regex multiline_comments;
    static const std::regex singleline_comments;
    static const std::regex white_space;
    static const std::regex extra_space;
    static const std::regex patch_declaration;
    static const std::regex struct_declaration;
};
