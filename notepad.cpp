#include <gtkmm.h>
#include <fstream>
#include <iostream>
#include <stack>
#include <ctime>
#include <sstream>

class Notepad : public Gtk::Window {
public:
    Notepad();
    ~Notepad() override = default;

protected:
    Gtk::Box m_vbox{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar m_menu_bar;
    Gtk::ScrolledWindow m_scrolled;
    Gtk::TextView m_text_view;
    Gtk::Statusbar m_statusbar;

    Glib::RefPtr<Gtk::TextBuffer> m_buffer;
    std::string m_filename;
    std::string m_last_find;
    std::stack<std::string> m_undo_stack, m_redo_stack;
    bool m_word_wrap = true;
    guint m_status_context_id;

    void on_new();
    void on_open();
    void on_save();
    void on_save_as();
    void on_exit();
    void on_undo();
    void on_cut();
    void on_copy();
    void on_paste();
    void on_delete();
    void on_find();
    void on_find_next();
    void on_time_date();
    void on_word_wrap();
    void on_font();
    void on_about();
    void update_status();
    void save_undo_state();
    void do_find(const Glib::ustring& text);
};

Notepad::Notepad() {
    set_title("Notepad");
    set_default_size(800, 600);

    // Dark mode + hide any extra window icons
    auto settings = Gtk::Settings::get_default();
    if (settings) {
        settings->property_gtk_application_prefer_dark_theme() = true;
        settings->property_gtk_decoration_layout() = Glib::ustring("menu:none");
    }

    m_buffer = Gtk::TextBuffer::create();
    m_text_view.set_buffer(m_buffer);
    m_text_view.set_wrap_mode(Gtk::WRAP_WORD);
    m_scrolled.add(m_text_view);

    m_status_context_id = m_statusbar.get_context_id("linecol");

    // Menu building
    auto file_menu = Gtk::manage(new Gtk::Menu());
    auto edit_menu = Gtk::manage(new Gtk::Menu());
    auto format_menu = Gtk::manage(new Gtk::Menu());
    auto help_menu = Gtk::manage(new Gtk::Menu());

    // File menu
    auto item_new    = Gtk::manage(new Gtk::MenuItem("_New", true));
    auto item_open   = Gtk::manage(new Gtk::MenuItem("_Open...", true));
    auto item_save   = Gtk::manage(new Gtk::MenuItem("_Save", true));
    auto item_saveas = Gtk::manage(new Gtk::MenuItem("Save _As...", true));
    auto item_exit   = Gtk::manage(new Gtk::MenuItem("E_xit", true));
    item_new->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_new));
    item_open->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_open));
    item_save->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_save));
    item_saveas->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_save_as));
    item_exit->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_exit));
    file_menu->append(*item_new); file_menu->append(*item_open);
    file_menu->append(*item_save); file_menu->append(*item_saveas);
    file_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    file_menu->append(*item_exit);

    // Edit menu
    auto item_undo = Gtk::manage(new Gtk::MenuItem("_Undo", true));
    item_undo->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_undo));
    edit_menu->append(*item_undo);
    edit_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto item_cut    = Gtk::manage(new Gtk::MenuItem("Cu_t", true));
    auto item_copy   = Gtk::manage(new Gtk::MenuItem("_Copy", true));
    auto item_paste  = Gtk::manage(new Gtk::MenuItem("_Paste", true));
    auto item_delete = Gtk::manage(new Gtk::MenuItem("_Delete", true));
    item_cut->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_cut));
    item_copy->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_copy));
    item_paste->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_paste));
    item_delete->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_delete));
    edit_menu->append(*item_cut); edit_menu->append(*item_copy);
    edit_menu->append(*item_paste); edit_menu->append(*item_delete);
    edit_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto item_find     = Gtk::manage(new Gtk::MenuItem("_Find...", true));
    auto item_findnext = Gtk::manage(new Gtk::MenuItem("Find _Next", true));
    auto item_time     = Gtk::manage(new Gtk::MenuItem("Time/_Date", true));
    item_find->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_find));
    item_findnext->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_find_next));
    item_time->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_time_date));
    edit_menu->append(*item_find); edit_menu->append(*item_findnext);
    edit_menu->append(*item_time);

    // Format menu
    auto item_wrap = Gtk::manage(new Gtk::CheckMenuItem("_Word Wrap", true));
    item_wrap->set_active(true);
    item_wrap->signal_toggled().connect(sigc::mem_fun(*this, &Notepad::on_word_wrap));
    format_menu->append(*item_wrap);

    auto item_font = Gtk::manage(new Gtk::MenuItem("_Font...", true));
    item_font->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_font));
    format_menu->append(*item_font);

    // Help menu
    auto item_about = Gtk::manage(new Gtk::MenuItem("_About Notepad", true));
    item_about->signal_activate().connect(sigc::mem_fun(*this, &Notepad::on_about));
    help_menu->append(*item_about);

    // Top menu bar (fixed with true for mnemonics — hides underlines)
    auto mfile   = Gtk::manage(new Gtk::MenuItem("_File", true));    mfile->set_submenu(*file_menu);   m_menu_bar.append(*mfile);
    auto medit   = Gtk::manage(new Gtk::MenuItem("_Edit", true));    medit->set_submenu(*edit_menu);   m_menu_bar.append(*medit);
    auto mformat = Gtk::manage(new Gtk::MenuItem("F_ormat", true));  mformat->set_submenu(*format_menu); m_menu_bar.append(*mformat);
    auto mhelp   = Gtk::manage(new Gtk::MenuItem("_Help", true));    mhelp->set_submenu(*help_menu);   m_menu_bar.append(*mhelp);

    m_vbox.pack_start(m_menu_bar, Gtk::PACK_SHRINK);
    m_vbox.pack_start(m_scrolled);
    m_vbox.pack_start(m_statusbar, Gtk::PACK_SHRINK);
    add(m_vbox);

    // Signals
    m_buffer->signal_changed().connect(sigc::mem_fun(*this, &Notepad::save_undo_state));
    m_buffer->signal_insert().connect([this](const Gtk::TextIter&, const Glib::ustring&, int){ update_status(); }, false);
    m_buffer->signal_erase().connect([this](const Gtk::TextIter&, const Gtk::TextIter&){ update_status(); }, false);

    // Keyboard shortcuts
    auto accel = Gtk::AccelGroup::create();
    add_accel_group(accel);
    item_new->add_accelerator("activate", accel, GDK_KEY_n, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item_open->add_accelerator("activate", accel, GDK_KEY_o, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item_save->add_accelerator("activate", accel, GDK_KEY_s, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item_undo->add_accelerator("activate", accel, GDK_KEY_z, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item_find->add_accelerator("activate", accel, GDK_KEY_f, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item_findnext->add_accelerator("activate", accel, GDK_KEY_F3, Gdk::ModifierType(0), Gtk::ACCEL_VISIBLE);
    item_time->add_accelerator("activate", accel, GDK_KEY_F5, Gdk::ModifierType(0), Gtk::ACCEL_VISIBLE);

    show_all_children();
    m_statusbar.push("Line 1, Column 1", m_status_context_id);
}

// All functions
void Notepad::on_new() { m_buffer->set_text(""); m_filename.clear(); set_title("Notepad"); m_undo_stack = std::stack<std::string>{}; m_redo_stack = std::stack<std::string>{}; }
void Notepad::on_exit() { hide(); }

void Notepad::on_open() {
    Gtk::FileChooserDialog dlg("Open File", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(*this);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Open", Gtk::RESPONSE_OK);
    if (dlg.run() == Gtk::RESPONSE_OK) {
        std::ifstream f(dlg.get_filename());
        std::stringstream ss; ss << f.rdbuf();
        m_buffer->set_text(ss.str());
        m_filename = dlg.get_filename();
        set_title("Notepad - " + Glib::filename_display_basename(m_filename));
        m_undo_stack = std::stack<std::string>{};
        m_redo_stack = std::stack<std::string>{};
        save_undo_state();
    }
}

void Notepad::on_save() { if (m_filename.empty()) on_save_as(); else { std::ofstream f(m_filename); f << m_buffer->get_text(); } }
void Notepad::on_save_as() {
    Gtk::FileChooserDialog dlg("Save As", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dlg.set_transient_for(*this);
    dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("_Save", Gtk::RESPONSE_OK);
    if (dlg.run() == Gtk::RESPONSE_OK) { m_filename = dlg.get_filename(); on_save(); set_title("Notepad - " + Glib::filename_display_basename(m_filename)); }
}

void Notepad::on_undo() { if (!m_undo_stack.empty()) { m_redo_stack.push(m_buffer->get_text()); m_buffer->set_text(m_undo_stack.top()); m_undo_stack.pop(); } }
void Notepad::on_cut()    { m_buffer->cut_clipboard(Gtk::Clipboard::get()); }
void Notepad::on_copy()   { m_buffer->copy_clipboard(Gtk::Clipboard::get()); }
void Notepad::on_paste()  { m_buffer->paste_clipboard(Gtk::Clipboard::get()); }
void Notepad::on_delete() { m_buffer->erase_selection(true, true); }

void Notepad::on_find() {
    Gtk::Dialog dlg("Find", *this, true);
    Gtk::Entry entry;
    dlg.get_content_area()->pack_start(entry);
    dlg.add_button("Find Next", Gtk::RESPONSE_OK);
    dlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    entry.signal_activate().connect([&](){ dlg.response(Gtk::RESPONSE_OK); });
    dlg.show_all();
    if (dlg.run() == Gtk::RESPONSE_OK && !entry.get_text().empty()) { m_last_find = entry.get_text(); do_find(m_last_find); }
}
void Notepad::on_find_next() { if (!m_last_find.empty()) do_find(m_last_find); }

void Notepad::do_find(const Glib::ustring& text) {
    auto start = m_buffer->get_insert()->get_iter();
    Gtk::TextIter mstart, mend;
    if (start.forward_search(text, Gtk::TEXT_SEARCH_TEXT_ONLY, mstart, mend)) {
        m_buffer->select_range(mstart, mend);
        m_text_view.scroll_to(mstart);
    }
}

void Notepad::on_time_date() {
    std::time_t t = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M %d/%m/%Y", std::localtime(&t));
    m_buffer->insert_at_cursor(buf);
}

void Notepad::on_word_wrap() { m_word_wrap = !m_word_wrap; m_text_view.set_wrap_mode(m_word_wrap ? Gtk::WRAP_WORD : Gtk::WRAP_NONE); }

void Notepad::on_font() {
    Gtk::FontChooserDialog dlg("Choose Font", *this);
    if (dlg.run() == Gtk::RESPONSE_OK)
        m_text_view.override_font(Pango::FontDescription(dlg.get_font()));
}

void Notepad::on_about() {
    Gtk::MessageDialog dlg(*this, "About Notepad", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
    dlg.set_secondary_text("Simple dark-mode Windows 7-style Notepad\nC++ + GTKmm 3");
    dlg.run();
}

void Notepad::update_status() {
    auto it = m_buffer->get_insert()->get_iter();
    int line = it.get_line() + 1;
    int col  = it.get_line_offset() + 1;
    m_statusbar.pop(m_status_context_id);
    m_statusbar.push("Line " + std::to_string(line) + ", Column " + std::to_string(col), m_status_context_id);
}

void Notepad::save_undo_state() {
    std::string txt = m_buffer->get_text();
    if (m_undo_stack.empty() || txt != m_undo_stack.top()) {
        m_undo_stack.push(txt);
        if (m_undo_stack.size() > 100) m_undo_stack.pop();
    }
}

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.example.notepad");
    Notepad win;
    return app->run(win);
}
