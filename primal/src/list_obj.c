void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode,
                       BOOL show);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                      BOOL show)
{
  struct obj_data *i;
  bool found;
  sh_int *unique, item_num;
  struct obj_data **u_item_ptrs;
  int size=0, num, num_unique;

  for (i = list; i; i->next_content, size++);

  if ((unique = malloc(size*2*sizeof(sh_int))) == NULL)
    list_obj_to_char2(list, ch, mode, show);
  else if ((u_item_ptrs = malloc(size*sizeof(struct obj_data*))) == NULL)
  {
    for (num=0; num<size; num++)
      free(unique+num);
    list_obj_to_char2(list, ch, mode, show);
  }
  else
  {
    num_unique=0;
    for (i = list; i; i->next_content)
    {
      item_num = i->item_number;
      found=FALSE;
      num=num_unique;
      while (num) {
        if (unique[(num-1)*2] == item_num)
        {
          found=TRUE;
          unique[(num-1)*2+1]++;
        }
        num--;
      }
      if (!found)
      {
        u_item_ptrs[num_unique]=i;
        unique[num_unique*2]=item_num;
        unique[num_unique*2+1]=1;
        num_unique++;
      }
    }
    found = FALSE;
    for (num=0; num<num_unique; num++)
      if (CAN_SEE_OBJ(ch,u_item_ptrs[num]))
      {
        sprintf(buf2, "[%d] ", unique[num*2+1]);
        send_to_char(buf2, ch);
        show_obj_to_char(u_item_ptrs[num], ch, mode);
        found = TRUE;
      }
    if (!found && show)
      send_to_char(" Nothing.\r\n", ch);
    for (num=0; num<size; num++)
    {
      free(unique+num);
      free(u_item_ptrs+num);
    }
  }
}

void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode,
                       BOOL show)
{
  struct obj_data *i;
  bool found;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
    if (CAN_SEE_OBJ(ch, i)) {
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}
