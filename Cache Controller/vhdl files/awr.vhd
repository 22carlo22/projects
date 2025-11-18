library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity awr is
    Port ( addr : in  STD_LOGIC_VECTOR (15 downto 0);
			  clk : in STD_LOGIC;
           index : out  STD_LOGIC_VECTOR (2 downto 0);
           tag : out  STD_LOGIC_VECTOR (7 downto 0);
           offset : out  STD_LOGIC_VECTOR (4 downto 0));
end awr;

architecture Behavioral of awr is
	signal temp : STD_LOGIC;
begin
	temp <= not clk;
	process(temp)
	begin
		if(temp'Event and temp = '1') then
			offset <= addr(4 downto 0);
			index <= addr(7 downto 5);
			tag <= addr(15 downto 8);
		end if;
	end process;

end Behavioral;
