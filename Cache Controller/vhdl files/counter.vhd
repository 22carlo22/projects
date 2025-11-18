library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity counter is
    Port ( inc : in  STD_LOGIC;
			  clk : in STD_LOGIC;
           output : out  STD_LOGIC_VECTOR (4 downto 0);
           bit0 : out  STD_LOGIC;
           overflow : out  STD_LOGIC);
end counter;

architecture Behavioral of counter is
	signal counter:std_logic_vector(6 downto 0) := (others => '0');
	signal ovf:std_logic := '0';
begin
	process(clk, inc)
	begin
		if(inc = '0') then
			counter <= (others => '0');
			ovf <= '0';
		elsif(clk'Event and clk = '1') then
			if(inc = '1') then 
				if(counter  = "1111111") then
					ovf <= '1';
				end if;
				
				if(counter(1 downto 0) = "01") then 
					counter(1 downto 0) <= "11";
				else
				   counter <= counter + '1';
				end if;
			end if;
		end if;
	end process;
			
	
	output <= counter(6 downto 2);
	bit0 <= counter(1);
	overflow <= ovf;

end Behavioral;
