library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity mux2to1_8bus is
    Port ( s : in  STD_LOGIC;
           i0 : in  STD_LOGIC_VECTOR (7 downto 0);
           i1 : in  STD_LOGIC_VECTOR (7 downto 0);
           o : out  STD_LOGIC_VECTOR (7 downto 0));
end mux2to1_8bus;

architecture Behavioral of mux2to1_8bus is

begin
	process(s, i0, i1)
	begin
		if(s = '1') then
			o <= i1;
		else
			o <= i0;
		end if;
	end process;


end Behavioral;
