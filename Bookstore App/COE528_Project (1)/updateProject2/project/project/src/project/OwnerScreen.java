/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package project;

public abstract class OwnerScreen extends ScreenType{
    public OwnerScreen(){
        super(new Books(), new Customers());
    }
}
