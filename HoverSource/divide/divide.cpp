

#include <iostream.h>
#include <conio.h>

int GetLesserPower( int bottom, int* exp ){

	int _exp=0;
	int power=1;
	int halfBottom = bottom >> 1;

	while( power <= halfBottom ){
		power <<= 1;
		_exp++;
	}

	*exp = _exp;
	return power;
}

int Divide( int num, int denom ){

	int exp, powerOfTwo;
	powerOfTwo = GetLesserPower( denom, &exp );

	int newNum, newDenom;
	newNum = ( denom - powerOfTwo ) * num;
	newDenom = denom << exp;

	if( newNum < 0 || newDenom < 0 ){
		cout << num << "/" << (1 << exp) << " - [overflow]";
		return ( num >> exp );
	}
	else if( newNum <= newDenom ){
		cout << num << "/" << (1 << exp) << " ( - " << newNum << "/" << newDenom << ")";
		return ( num >> exp );
	}
	else{
		cout << num << "/" << (1 << exp) << " - ";
		cout.flush();
		getch();
		return ( num >> exp ) - Divide( newNum, newDenom );
	}
}

void main(){

	int num, denom, result;

	do{
		cout << "\n\nEnter numerator and denominator:";
		cin >> num >> denom;
		if( !num ) break;
		cout << num << "/" << denom << "\n= ";
		result = Divide( num, denom );
		cout << "\n= " << result;
		cout << "\ncorrect result: " << ( num / denom );
	}while( 1 );

}