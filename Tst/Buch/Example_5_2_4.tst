LIB "tst.lib";
tst_init();

proc MonomialHilbertPoincare(ideal I)
{
  I    = interred(I); //computes a minimal set of generators
  int s= size(I);     //of the monomial ideal I
 
  if(I[1]     ==0){return(1);}            //I=<0>
  if(I[1]     ==1){return(0);}            //I=<1>
  if(deg(I[s])==1){return((1-var(1))^s);} //I is generated by
                                          //s of the {var(j)}
  int j=2;                                            
  while(leadexp(I[s])[j]==0){j++;}        //I[s]=var(j)*m
  return(MonomialHilbertPoincare(I+var(j))
        +var(1)*MonomialHilbertPoincare(quotient(I,var(j))));
}

ring A=0,(t,x,y,z),dp;
ideal I=x5y2,x3,y3,xy4,xy7;

MonomialHilbertPoincare(I);

intvec v=hilb(std(I),1);
v;

tst_status(1);$
