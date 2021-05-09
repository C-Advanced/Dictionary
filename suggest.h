// Hàm  này dùng khi chọn một từ ở mục đề xuất của searchEntry
// Sau đó đặt từ đó vào searchEntry.
gboolean match_selected_for_search(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gchar *word; // Khởi tạo biến word
    gtk_tree_model_get(model, iter, 0, &word, -1); // Get tree model

    gtk_entry_set_text(GTK_ENTRY(searchEntry), word); //set text vao searchEntry
    // update lookup field

    lookUp(word); // Gọi hàm tìm kiếm từ
    return TRUE;
}

gboolean match_selected_for_delete(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gchar *word;                                   // Khởi tạo biến word
    gtk_tree_model_get(model, iter, 0, &word, -1); // Get tree model

    // update lookup field
    gtk_entry_set_text(entryDeleteWord, word); // Đặt  word vào trong bộ đệm (buffer) của searchEntry

    return TRUE;
}

// Hàm cập nhật lại các đề xuất sau khi thực kiện các thao tác thêm, xóa từ, tải từ dũ liệu
void wordListForSuggest(GtkEntry *entry)
{
    GtkListStore *listStore;
    GtkTreeIter iter;
    char word[WORD_MAX_LEN]; // Khởi tạo word
    int i;

    GtkEntryCompletion *completion = gtk_entry_completion_new(); // Tạo 1 Entry Completion
    gtk_entry_completion_set_text_column(completion, 0);

    // Dòng này để kết nối tín hiệu xử lý khi ta chọn một từ trên đề xuất sẽ gọi đến hàm match_selected ở trên
    if (strcmp(gtk_widget_get_name(GTK_ENTRY(entry)), "searchEntry") == 0)
    {
        g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(match_selected_for_search), NULL);
    }
    else
    {
        g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(match_selected_for_delete), NULL);
    }

    listStore = gtk_list_store_new(1, G_TYPE_STRING); // Khởi tạo list store
    
    // Cho con trỏ BTA* eng_vie về vị trí đầu tiên trong BTree
    btpos(eng_vie, 1);
    //Sử dụng vòng lặp while để thêm từ vào list store
    while (bnxtky(eng_vie, word,  &i) == 0)
    {
        gtk_list_store_append(listStore, &iter);
        gtk_list_store_set(listStore, &iter, 0, word, -1);
    }

    //Set mode cho Entry Completion với list store
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(listStore));

    // Set completion cho 1 entry
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);
}
