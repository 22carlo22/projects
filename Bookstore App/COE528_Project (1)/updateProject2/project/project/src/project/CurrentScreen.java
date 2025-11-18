/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package project;


public class CurrentScreen {
    //Customer x = new Customer("Stanley","Tan",999);
    //private ScreenType screen = new CustomerStartScreen(x);
    //private ScreenType screen = new CustomersScreen();
    private ScreenType screen = new LogInScreen(); 
    
    public void setScreen(ScreenType s){
        screen = s;
        setScene();
    }
    
    public void setScene(){
        screen.display(this);
    }
}
