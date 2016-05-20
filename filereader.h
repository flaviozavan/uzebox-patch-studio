class FileReader {
  public:
    static bool read_patches(const wxString &fn,
        std::map<wxString, wxVector<long>> &data);

  private:
    static long string_to_long(const wxString &str);
    static bool read_patch_vals(const wxString &str, wxVector<long> &vals);
    static std::string clean_code(const std::string &code);

    static const std::map<wxString, long> defines;
    static const std::regex multiline_comments;
    static const std::regex singleline_comments;
    static const std::regex white_space;
    static const std::regex extra_space;
    static const std::regex patch_declaration;
};
