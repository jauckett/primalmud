{
  FOUND_OBJ_FILE = index($1, "objs")
  if (FOUND_OBJ_FILE > 0)
  {
    POS_DOT = index($1,".")
    POS_DOT--
    BASE_PATH = substr($1, 1, POS_DOT)
    printf("cp %s %sm.objs\n", $1, BASE_PATH) 
  }
}
