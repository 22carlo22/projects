/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package project;

public abstract class CustomerScreen extends ScreenType{
    private Customer customer;
    
    public CustomerScreen(Customer c){
        super(new Books());
        customer = c;
    }
    
    public Customer getCustomer(){
        return customer;
    }
}
