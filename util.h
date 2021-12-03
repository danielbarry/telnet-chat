int util_strLen(char* a);

int util_strLen(char* a){
  int x = 0;
  while(a[x++] != '\0');
  return x;
}
