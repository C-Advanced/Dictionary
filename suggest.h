// Hàm  này dùng khi chọn một từ ở mục đề xuất của searchEntry
// Sau đó đặt từ đó vào searchEntry.
gboolean match_selected(GtkEntryCompletion *widget,GtkTreeModel *model,GtkTreeIter *iter, app_widgets *wdgt) {
    gchar *word; // Khởi tạo biến word
    gtk_tree_model_get(model, iter, 0, &word, -1); // Get tree model

    // update lookup field
    gtk_entry_buffer_set_text(gtk_entry_get_buffer(searchEntry), word, -1); // Đặt  word vào trong bộ đệm (buffer) của searchEntry

    lookUp(wdgt, word); // Gọi hàm tìm kiếm từ

    return TRUE;
}

// Hàm cập nhật lại các đề xuất sau khi thực kiện các thao tác thêm, xóa từ, tải từ dũ liệu
void wordListForSuggest(GtkWidget *gWidget)
{

    GtkListStore *listStore;
    GtkTreeIter iter;
    char word[WORD_MAX_LEN]; // Khởi tạo word
    char mean[MEAN_MAX_LEN]; // Khởi tạo mean
    int i;

    GtkEntryCompletion *completion = gtk_entry_completion_new(); // Tạo 1 Entry Completion
    gtk_entry_completion_set_text_column(completion,0); 
    gtk_entry_set_completion(GTK_ENTRY(gWidget),completion);  // Set completion cho 1 entry

    // Dòng này để kết nối tín hiệu xử lý khi ta chọn một từ trên đề xuất sẽ gọi đến hàm match_selected ở trên
    g_signal_connect(G_OBJECT(completion),"match-selected",G_CALLBACK(match_selected),NULL);
    listStore = gtk_list_store_new(1,G_TYPE_STRING);  // Khởi tạo list store
    btpos(eng_vie,1); // Cho con trỏ BTA* eng_vie về vị trí đầu tiên trong BTree

    //Sử dụng vòng lặp while để thêm từ vào list store 
    while(btseln(eng_vie,word,mean, MEAN_MAX_LEN, &i)==0)
    {
        gtk_list_store_append(listStore,&iter);
        gtk_list_store_set(listStore,&iter,0,word,-1);
    }

    //Set mode cho Entry Completion với list store 
    gtk_entry_completion_set_model(completion,GTK_TREE_MODEL(listStore));
}
