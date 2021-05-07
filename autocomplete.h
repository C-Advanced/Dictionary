void match_selected(GtkEntryCompletion *widget,GtkTreeModel *model,GtkTreeIter *iter, app_widgets *wdgt)
{
    GValue value = {0, };
    const char *word;
    gtk_tree_model_get_value(model,iter, 0,&value);
    word = g_value_get_string(&value);
    gtk_entry_set_text(GTK_ENTRY(searchEntry),word);
    g_value_unset(&value);
}

void wordListForAutoComplete(GtkWidget *gWidget)
{

    GtkListStore *listStore;
    GtkTreeIter iter;
    char word[1000];
    char mean[1000];
    int i;

    GtkEntryCompletion *completion = gtk_entry_completion_new();
    gtk_entry_completion_set_text_column(completion,0);
    gtk_entry_set_completion(GTK_ENTRY(gWidget),completion);

    listStore = gtk_list_store_new(1,G_TYPE_STRING);
    btpos(eng_vie,1);
    while(btseln(eng_vie,word,mean,1000,&i)==0)
    {
        gtk_list_store_append(listStore,&iter);
        gtk_list_store_set(listStore,&iter,0,word,-1);
    }
    gtk_entry_completion_set_model(completion,GTK_TREE_MODEL(listStore));
}
