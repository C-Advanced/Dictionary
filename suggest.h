// Hàm  này dùng khi chọn một từ ở mục đề xuất của searchEntry
// Sau đó đặt từ đó vào searchEntry.
gboolean match_selected_for_search(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gchar *word;                                   // Khởi tạo biến word
    gtk_tree_model_get(model, iter, 0, &word, -1); // Hàm lấy word từ một ô trong một bảng. Ở đây bảng là "GtkListStore" đã được chuyển thành GtkTreeModel

    gtk_entry_set_text(GTK_ENTRY(searchEntry), word); //set text vao searchEntry
    // update lookup field

    lookUp(word); // Gọi hàm tìm kiếm từ
    return TRUE;
}

gboolean match_selected_for_delete(GtkEntryCompletion *completion, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gchar *word;                                   // Khởi tạo biến word
    gtk_tree_model_get(model, iter, 0, &word, -1); // Get tree model

    // update lookup field
    gtk_entry_set_text(entryDeleteWord, word); // Đặt  word vào trong bộ đệm (buffer) của searchEntry

    return TRUE;
}

// Hàm xay dung va cap nhat lai các đề xuất sau khi thực kiện các thao tác thêm, xóa từ, tải từ dũ liệu
void buildListSuggest(GtkEntry *entry)
{
    GtkListStore *listStore;

    //Dùng để trỏ đến một hàng trong ListStore
    GtkTreeIter iter;

    //Khởi tạo word
    char word[WORD_MAX_LEN];
    int i;

    //Tạo 1 Entry Completion
    GtkEntryCompletion *completion = gtk_entry_completion_new();

    // Dòng này để kết nối tín hiệu xử lý khi ta chọn một từ trên đề xuất sẽ gọi đến hàm match_selected ở trên
    if (strcmp(gtk_widget_get_name(GTK_ENTRY(entry)), "searchEntry") == 0)
    {
        g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(match_selected_for_search), NULL);
    }
    else
    {
        g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(match_selected_for_delete), NULL);
    }

    //Khởi tạo list store với tham số đầu tiên là số cột và tham số thứ hai là kiểu cho các dữ liệu ở cột đó
    listStore = gtk_list_store_new(1, G_TYPE_STRING);

    //Cho con trỏ BTA* eng_vie về vị trí đầu tiên trong BTree
    btpos(eng_vie, 1);
    //Sử dụng vòng lặp while để thêm từ vào list store
    while (bnxtky(eng_vie, word, &i) == 0)
    {
        //Thêm từ
        //Đầu tiên phải cho iter trỏ đến hàng tiếp theo
        gtk_list_store_append(listStore, &iter);

        //Sau đó, gán dữ liệu vào hàng đó nhờ vào biến iter
        //Các tham số của hàm: liststore, iter, số thự tự cột(ở đây cột đâu tiên là cột 0), nội dung của hàng đó, -1. 
        gtk_list_store_set(listStore, &iter, 0, word, -1);
    }

    //Chọn cột để làm dữ liệu cho hoàn thành
    gtk_entry_completion_set_text_column(completion, 0);

    //Set model cho Entry Completion với list store
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(listStore));

    //Set completion cho 1 entry
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);
}
