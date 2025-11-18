/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package project;

import java.util.ArrayList;


public class Books extends Data{
    private static ArrayList<Book> books = new ArrayList<>();
    
    public void add(Book b){
        books.add(b);
    }
    
    public void delete(Book b){
        books.remove(b);
    }
    
    public ArrayList<Book> getAll(){
        return (ArrayList<Book>)books;
    }
}
