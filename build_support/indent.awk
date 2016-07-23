function repeat(str, n, rep, i)
{
  for (; i<n; i++)
  {
    rep = rep str
  }
  return rep
}
BEGIN { ind = 0; };
$1 == "EXIT" { ind -= 2; };
{ print repeat(" ", ind) $0; };
$1 == "ENTRY" { ind += 2; } ;

