

int main(){
    float amount_usd = (amount_cad * 81.7)/100.0;
    char memo[32];
    char purpose;
    strcpy(purpose,0X99);
    printf("Purpose: %s",purpose); 
    strcpy(memo, "Payment for: ");
    strcat(memo, purpose);
    printf("Memo: %s\n",memo);

}